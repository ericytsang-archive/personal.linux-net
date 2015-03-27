#include "Host.h"
#include "net_helper.h"
#include "select_helper.h"
#include "Message.h"

#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <strings.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <vector>
#include <set>

/**
 * communicate to the receive thread through the receive pipe, to read a socket
 *   from the receive pipe, and add it to the set of sockets to select.
 */
#define ADD_SOCK 0

/**
 * communicate to the receive thread through the receive pipe, to read a socket
 *   from the receive pipe, and remove it from the set of sockets to select.
 */
#define RM_SOCK 1

using namespace Net;

// forward declarations
static void fatal_error(const char* errstr);

/**
 * constructs a new {Server}.
 */
Host::Host()
{
    svrSock = -1;
    listenThread  = 0;
    receiveThread = 0;
    startReceiveRoutine();
}

/**
 * Clean up the Server on destruction.
 */
Host::~Host()
{
    stopReceiveRoutine();
}

/**
 * initializes the server to listen for incoming connections on the
 *   given port
 *
 * @param  port to connect to
 *
 * @return integer indicating the outcome of the operation
 */
int Host::startListeningRoutine(short port)
{
    // open the server socket
    if(listenThread == 0)
    {
        if((svrSock = make_tcp_server_socket(port)) == -1)
        {
            return SOCK_OP_FAIL;
        }
    }

    return startRoutine(&listenThread,listenRoutine,listenPipe,this);
}

/**
 * stops server, and closes all connections connected with the server.
 *
 * @return 0 upon success; -1 on failure. check errno for details.
 */
int Host::stopListeningRoutine()
{
    return stopRoutine(&listenThread,listenPipe);
}

/**
 * sends a message to the remote host, using the protocol that hosts use.
 *
 * @param socket socket to send the data to.
 * @param msg message to send to the remote host.
 */
void Host::send(int socket, Message msg)
{
    write(socket,&msg.type,sizeof(msg.type));
    write(socket,&msg.len,sizeof(msg.len));
    write(socket,msg.data,msg.len);
}

int Host::connect(char* remoteName, short remotePort)
{
    // connect to remote host
    int socket = make_tcp_client_socket(remoteName,0,remotePort,0);

    if(socket != -1)
    {
        // communicate to receive thread that a new socket is connected
        char commandType = ADD_SOCK;
        write(receivePipe[1],&commandType,sizeof(commandType));
        write(receivePipe[1],&socket,sizeof(socket));
    }

    return (socket != -1) ? SUCCESS : SOCK_OP_FAIL;
}

void Host::disconnect(int socket)
{
    // communicate to receive thread to remove an existing socket
    char commandType = RM_SOCK;
    write(receivePipe[1],&commandType,sizeof(commandType));
    write(receivePipe[1],&socket,sizeof(socket));
}

void Host::onConnect(int socket)
{
    printf("server: socket %d connected\n",socket);
}

void Host::onMessage(int socket, Message msg)
{
    printf("server: socket %d: msg.type: %d, msg.data: ",socket,msg.type);
    for(int i = 0; i < msg.len; ++i)
    {
        printf("%c",((char*)msg.data)[i]);
    }
    printf("\n");
}

void Host::onDisconnect(int socket, int remote)
{
    printf("server: socket %d disconnected by %s host\n",
        socket,remote?"remote":"local");
}

int Host::startReceiveRoutine()
{
    return startRoutine(&receiveThread,receiveRoutine,receivePipe,this);
}

int Host::stopReceiveRoutine()
{
    return stopRoutine(&receiveThread,receivePipe);
}

/**
 * starts the passed routine on a new thread. the thread id will be assigned to
 *   the passed thread id pointer.
 *
 * @function   Host::startRoutine
 *
 * @date       2015-03-18
 *
 * @revision   none
 *
 * @designer   EricTsang
 *
 * @programmer EricTsang
 *
 * @note       none
 *
 * @signature  int Host::startRoutine(pthread_t* thread, void* routine, int*
 *   controlPipe, void* params)
 *
 * @param      thread pointer to the thread id variable.
 * @param      routine function to execute on the thread.
 * @param      controlPipe pointer to an int[2] that holds the file descriptors
 *   of a unnamed FIFO unnamed pipe.
 * @param      params parameters to pass to the new thread.
 *
 * @return     [file_header] [class_header] [description]
 */
int Host::startRoutine(pthread_t* thread, void*(*routine)(void*), int* controlPipe, void* params)
{
    // return immediately if the routine is already running
    if(*thread != 0)
    {
        return INVALID_OPERATION;
    }

    // create the control pipe
    if(pipe(controlPipe) == SYS_ERROR)
    {
        fatal_error("failed to create the control pipe");
    }

    // start the thread
    pthread_create(thread,0,routine,params);
    return SUCCESS;
}

/**
 * stops a thread from executing a routine by communicating to it through an IPC
 *   pipe.
 *
 * @function   Host::stopRoutine
 *
 * @date       2015-03-18
 *
 * @revision   none
 *
 * @designer   EricTsang
 *
 * @programmer EricTsang
 *
 * @note       none
 *
 * @signature  int Host::stopRoutine(pthread_t* thread, int* controlPipe)
 *
 * @param      thread pointer to a thread id variable.
 * @param      controlPipe pointer to an integer array of size 2, used to hold
 *   the read and write file descriptors of a pipe.
 *
 * @return     integer values indicating the outcome of the operation.
 */
int Host::stopRoutine(pthread_t* thread, int* controlPipe)
{
    // return immediately if the routine is already stopped
    if(*thread == 0)
    {
        return INVALID_OPERATION;
    }

    // close the control pipe which terminates the thread
    close(controlPipe[1]);
    pthread_join(*thread,0);

    // set thread to 0, so we know it's terminated
    *thread = 0;
    return SUCCESS;
}

/**
 * function run on a thread. it polls the server socket, accepting connections.
 *
 * @param params thread parameters; points to the calling server instance.
 */
void* Host::listenRoutine(void* params)
{
    printf("listenroutine started...\n");
    fflush(stdout);
    // parse thread parameters
    Host* dis = (Host*) params;

    int terminateThread = 0;

    // set up the socket set & client list
    Files files;
    files_init(&files);

    // add the server socket and control pipe to the select set
    files_add_file(&files,dis->svrSock);
    files_add_file(&files,dis->listenPipe[0]);

    // accept any connection requests, and create a session for each
    while(!terminateThread && dis->svrSock != -1)
    {
        // wait for an event on any socket to occur
        if(files_select(&files) == -1)
        {
            fatal_error("failed on select");
        }

        // loop through sockets, and handle them
        for(auto socketIt = files.fdSet.begin(); socketIt != files.fdSet.end();
            ++socketIt)
        {
            int curSock = *socketIt;

            // if this socket doesn't have any activity, move on to next socket
            if(!FD_ISSET(curSock,&files.selectFds))
            {
                continue;
            }

            // handle socket activity depending on which socket it is
            if(curSock == dis->svrSock)
            {
                /*
                 * this is the server socket, try to accept a connection.
                 *
                 * if the operation fails, end the server thread, because when
                 *   accept fails, it means that the server socket is closed.
                 *
                 * if accept succeeds, add it to the select set, and continue
                 *   looping...
                 */

                // accept the connection
                int newSock;
                if((newSock = accept(dis->svrSock,0,0)) == -1)
                {
                    // accept failed; server socket closed, terminate thread
                    terminateThread = 1;
                }
                else
                {
                    // accept success; add the socket to the receive thread.
                    char commandType = ADD_SOCK;
                    write(dis->receivePipe[1],&commandType,sizeof(commandType));
                    write(dis->receivePipe[1],&newSock,sizeof(newSock));
                }
            }

            if(curSock == dis->listenPipe[0])
            {
                /*
                 * this is the control pipe. whenever anything happens on the
                 *   control pipe, it means it's time for the server to
                 *   shutdown; break out of the server loop.
                 */

                 terminateThread = 1;
            }
        }
    }

    // close all file descriptors before terminating
    for(auto socketIt = files.fdSet.begin(); socketIt != files.fdSet.end();
        ++socketIt)
    {
        close(*socketIt);
    }

    printf("listenroutine stopped...\n");
    fflush(stdout);

    return 0;
}

void* Host::receiveRoutine(void* params)
{
    printf("receiveroutine started...\n");
    fflush(stdout);

    // parse thread parameters
    Host* dis = (Host*) params;

    // used to break the while loop
    int terminateThread = 0;

    // set of sockets that have been shutdown from local host
    std::set<int> shutdownSocks;

    // set up the socket set & client list
    Files files;
    files_init(&files);

    // add the server socket and control pipe to the select set
    files_add_file(&files,dis->receivePipe[0]);

    // accept any connection requests, and create a session for each
    while(!terminateThread)
    {
        // wait for an event on any socket to occur
        if(files_select(&files) == -1)
        {
            fatal_error("failed on select");
        }

        // loop through sockets, and handle them
        for(auto socketIt = files.fdSet.begin(); socketIt != files.fdSet.end();
            ++socketIt)
        {
            int curSock = *socketIt;

            // if this socket doesn't have any activity, move on to next socket
            if(!FD_ISSET(curSock,&files.selectFds))
            {
                continue;
            }

            // handle socket activity depending on which socket it is
            if(curSock == dis->receivePipe[0])
            {
                /*
                 * this is the control pipe. try to read from the control pipe.
                 *
                 * if the control pipe is closed, the client is being deleted;
                 *   this client thread should terminate.
                 *
                 * if the control pipe is read, that means that a new socket is
                 *   now connected, and needs to be added to the selection set.
                 */

                char cmdType;
                if(read_file(dis->receivePipe[0],&cmdType,sizeof(cmdType)) == 0)
                {
                    // pipe closed; the client is being deleted, thread should
                    // terminate
                    terminateThread = 1;
                }
                else
                {
                    // pipe read; read a socket from the pipe, and depending on
                    // the cmdType, do something with it
                    int socket;
                    read_file(dis->receivePipe[0],&socket,sizeof(socket));
                    switch(cmdType)
                    {
                    case ADD_SOCK:
                        files_add_file(&files,socket);
                        dis->onConnect(socket);
                        break;
                    case RM_SOCK:
                        if(files.fdSet.find(socket) != files.fdSet.end())
                        {
                            shutdown(socket,SHUT_RDWR);
                            shutdownSocks.insert(socket);
                        }
                        break;
                    }
                }
            }
            else
            {
                /*
                 * this is the client socket; try to read from the socket
                 *
                 * if read fails, it means that the socket is closed, remove it
                 *   from the select set, call a callback, and continue looping.
                 *
                 * if read succeeds, read until socket is empty, and call
                 *   callback, then continue looping.
                 */

                // read from socket
                Message msg;
                if(read_file(curSock,&msg.type,sizeof(msg.type)) == 0)
                {
                    // socket closed; remove from select set, and call callback
                    files_rm_file(&files,curSock);
                    int remote = (shutdownSocks.erase(curSock) == 0
                        && close(curSock) == 0);
                    dis->onDisconnect(curSock,remote);
                    close(curSock);
                }
                else
                {
                    // socket read; read more from socket, and call callback
                    read_file(curSock,&msg.len,sizeof(msg.len));

                    char* buffer = (char*) malloc(msg.len);
                    read_file(curSock,buffer,msg.len);
                    msg.data = buffer;

                    dis->onMessage(curSock,msg);

                    free(buffer);
                }
                break;
            }
        }
    }

    // close all sockets before terminating
    for(auto socketIt = files.fdSet.begin(); socketIt != files.fdSet.end();
        ++socketIt)
    {
        int curSock = *socketIt;
        close(curSock);
        if(curSock != dis->receivePipe[0])
        {
            dis->onDisconnect(curSock,0);
        }
    }

    printf("receiveroutine stopped...\n");
    fflush(stdout);

    return 0;
}

static void fatal_error(const char* errstr)
{
    perror(errstr);
    exit(errno);
}
