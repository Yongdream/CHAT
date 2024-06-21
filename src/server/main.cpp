#include "chatserver.hpp"
#include "chatservice.hpp"
#include <signal.h>
#include <iostream>

using namespace std;

void resetHandler(int){
    ChatService::instance()->reset();
    exit(0);
}
 
int main(int argc, char **argv)
{
    // 集群服务器端口
    uint16_t defaultPort = 6000;
    uint16_t fallbackPort = 6002;

    uint16_t port;

    if (argc < 2)
    {
        port = defaultPort;
    }
    else
    {
        port = (argc > 1) ? atoi(argv[1]) : defaultPort;

        if (port != 6000 && port != 6002)
        {
            cerr << "Invalid port specified! Only 6000 and 6002 are allowed." << endl;
            return -1;
        }
    }

    //捕捉 Ctrl+C 信号
    signal(SIGINT,resetHandler);

    EventLoop loop;
    InetAddress addr("192.168.122.129", port);
    ChatServer server(&loop, addr, "ChatServer");
 
    server.start();
    loop.loop();
    return 0;
}
