#include <string.h>

#include "Server.h"
#include "Message.h"
#include "protocol.h"

Server::Server()
{
}

Server::~Server()
{
}

void Server::onConnect(int socket)
{
    Host::onConnect(socket);
}

void Server::onMessage(int socket, Net::Message msg)
{
    Host::onMessage(socket,msg);
    switch(msg.type)
    {
    case SHOW_MSG:
        onMessage(socket,(char*)msg.data);
        break;
    case CHECK_USR_NAME:
        onCheckUserName(socket,(char*)msg.data);
        break;
    }
}

void Server::onDisconnect(int socket, int remote)
{
    Host::onDisconnect(socket,remote);
}

void Server::onClientConnect(int clntSock, char* clientName)
{
    printf("%s has connected.\n",clientName);
    clients[clntSock] = clientName;

    // construct the chat message
    Net::Message msg;
    msg.type = ADD_CLIENT;
    msg.len  = strlen(clientName);
    msg.data = clientName;

    // send connect message to all clients
    for(auto client = clients.begin(); client != clients.end(); ++client)
    {
        auto curSock = (*client).first;
        send(curSock,msg);
    }
}

void Server::onClientDisconnect(int clntSock, char* clientName)
{
    printf("%s has disconnected.\n",clientName);
    clients.erase(clntSock);

    // construct the chat message
    Net::Message msg;
    msg.type = RM_CLIENT;
    msg.len  = strlen(clientName);
    msg.data = clientName;

    // send message to all clients except the one that sent it
    for(auto client = clients.begin(); client != clients.end(); ++client)
    {
        auto curSock = (*client).first;
        send(curSock,msg);
    }
}

void Server::onMessage(int clntSock, char* message)
{
    // print message
    printf("%s\n",message);

    // construct the chat message
    Net::Message msg;
    msg.type = SHOW_MSG;
    msg.len  = strlen(message);
    msg.data = message;

    // send message to all clients except the one that sent it
    for(auto client = clients.begin(); client != clients.end(); ++client)
    {
        auto curSock = (*client).first;
        if(curSock != clntSock)
        {
            send(curSock,msg);
        }
    }
}

void Server::onCheckUserName(int clntSock, char* newUsername)
{
    printf("onCheckUserName(%d,%s)\n",clntSock,newUsername);
    onClientConnect(clntSock,newUsername);
}

int main(void)
{
    Server* svr = new Server();

    svr->startListeningRoutine(7000);
    printf("server started\n");
    getchar();

    delete svr;
    printf("server stopped\n");

    return 0;
}
