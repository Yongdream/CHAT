// #include<iostream>
// #include "chatserver.hpp"

// using namespace std;

// int main(){
//     EventLoop loop;
//     InetAddress addr("192.168.122.lsof -i :8083 1234561",7000);
//     ChatServer server(&loop,addr,"ChatServer");

//     server.start();
//     loop.loop();
//     return 0;
// }


#include "chatserver.hpp"
 
int main(){
    EventLoop loop;
    InetAddress addr("192.168.122.129",6000);
    ChatServer server(&loop,addr,"ChatServer");
 
    server.start();
    loop.loop();
    return 0;
}
