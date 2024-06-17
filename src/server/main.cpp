#include "chatserver.hpp"
#include "chatservice.hpp"
#include<signal.h>

using namespace std;

void resetHandler(int){
    ChatService::instance()->reset();
    exit(0);
}
 
int main(){
    //捕捉 Ctrl+C 信号
    signal(SIGINT,resetHandler);

    EventLoop loop;
    InetAddress addr("192.168.122.129",6000);
    ChatServer server(&loop,addr,"ChatServer");
 
    server.start();
    loop.loop();
    return 0;
}
