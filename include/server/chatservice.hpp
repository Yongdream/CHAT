#ifndef CHATSERVICE_H
#define CHATSERVICE_H
 
#include <muduo/net/TcpConnection.h>
#include <unordered_map>    // 一个消息ID映射一个事件处理 
#include <functional>
#include <mutex>

using namespace std;
using namespace muduo;
using namespace muduo::net;
 
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "json.hpp"
#include "redis.hpp"

using json = nlohmann::json;
 
// 表示处理消息的事件回调方法类型，事件处理器，派发3个东西 
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;
 
// 聊天服务器业务类
class ChatService
{
public:
    // 获取单例对象的接口函数
    static ChatService *instance();
    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 私聊消息
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp timr);

    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 添加好友
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 注销业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);
    
    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

    // 客户端用户状态重置
    void reset();

    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);

    void handleRedisSubscribeMessage(int userid, string msg);
private:
    ChatService();//单例 
 
    //存储消息id和其对应的业务处理方法，消息处理器的一个表，写消息id对应的处理操作 
    unordered_map<int, MsgHandler> msgHandlerMap_;

    // 存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> userConnMap_;

    // 定义互斥锁 保证userConnMap_的线程安全
    mutex connMutex_;

    // 数据操作类对象
    UserModel userModel_;
    OfflineMsgModel offlineMsgModel_;
    FriendModel friendModel_;
    GroupModel groupModel_;

    // redis操作对象
    Redis redis_;
};
 
#endif