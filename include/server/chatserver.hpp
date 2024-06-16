#ifndef CHATSERVER_H
#define CHATSERBER_H
 
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
 
using namespace muduo;
using namespace muduo::net;
 
//聊天服务器的主类
class ChatServer
{
private:
    TcpServer _server;  // 组合muduo库，实现服务器功能的类对象
    EventLoop* _loop;   // 指向事件循环单元的指针
public:
    // 初始化聊天服务器对象
    ChatServer(EventLoop *loop,
               const InetAddress &listenAddr,
               const string &nameArg);
    // 启动服务
    void start();
 
private:
    // 上报链接相关信息回调
    void onConnection(const TcpConnectionPtr &);
    // 上报读写相关信息回调
    void onMessage(const TcpConnectionPtr &,
                   Buffer *,
                   Timestamp);       
};
 
#endif