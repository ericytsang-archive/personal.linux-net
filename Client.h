#include "Host.h"

namespace Net
{
    struct Message;
    class Host;
};

class Client : public Net::Host
{
public:
    Client();
    ~Client();
    void sendChatMessage(char* chatMsg);
protected:
    virtual void onConnect(int socket);
    virtual void onMessage(int socket, Net::Message msg);
    virtual void onDisconnect(int socket, int remote);
private:
    void onAddClient(char* clientName);
    void onRmClient(char* clientName);
    void onShowMessage(char* message);
    void onSetName(char* newUsername);
    char* name;
    /**
     * socket that's connected to the chat server.
     */
    int svrSock;
};
