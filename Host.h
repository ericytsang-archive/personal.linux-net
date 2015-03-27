#ifndef SERVER_H_
#define SERVER_H_

#include <map>
#include <pthread.h>

/**
 * indicates that a system call has failed.
 */
#define SYS_ERROR -1

/**
 * indicates that the operation has finished successfully.
 */
#define SUCCESS 0
/**
 * indicates that the operation is invalid for the object's current state.
 */
#define INVALID_OPERATION 1
/**
 * indicates that the operation failed due to a socket operation.
 */
#define SOCK_OP_FAIL 2

namespace Net
{
    struct Message;

    class Host
    {
    public:
        Host();
        virtual ~Host();
        int startListeningRoutine(short port);
        int stopListeningRoutine();
        void send(int socket, Message msg);
        int connect(char* remoteName, short remotePort);
        void disconnect(int socket);
    protected:
        virtual void onConnect(int socket);
        virtual void onMessage(int socket, Message msg);
        virtual void onDisconnect(int socket, int remote);
    private:
        int startReceiveRoutine();
        int stopReceiveRoutine();
        int startRoutine(pthread_t* thread, void*(*routine)(void*), int* controlPipe, void* params);
        int stopRoutine(pthread_t* thread, int* controlPipe);
        static void* listenRoutine(void* params);
        static void* receiveRoutine(void* params);

        /**
         * socket used to listen for new connections from.
         */
        int svrSock;

        /**
         * pipe used to communicate with the listenThread.
         */
        int listenPipe[2];

        /**
         * thread id for the thread that runs the listenRoutine.
         */
        pthread_t listenThread;

        /**
         * pipe used to communicate with the receiveThread.
         */
        int receivePipe[2];

        /**
         * thread id for the thread that runs the receiveRoutine.
         */
        pthread_t receiveThread;
    };
}

#endif
