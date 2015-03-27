#include <map>

#include "Host.h"

namespace Net
{
    struct Message;
    class Host;
};

class Server : public Net::Host
{
public:
    Server();
    ~Server();
protected:
    virtual void onConnect(int socket);
    virtual void onMessage(int socket, Net::Message msg);
    virtual void onDisconnect(int socket, int remote);
private:
    void onClientConnect(int clntSock, char* clientName);
    void onClientDisconnect(int clntSock, char* clientName);
    void onMessage(int clntSock, char* message);
    void onCheckUserName(int clntSock, char* newUsername);
    std::map<int,char*> clients;
};
