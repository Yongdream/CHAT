#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h> // muduo的日志 
#include <string>
#include <iostream>

using namespace std;
using namespace muduo;
 
// 获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}
 
// 构造方法，注册消息以及对应的Handler回调操作
ChatService::ChatService()
{
    //用户基本业务管理相关事件处理回调注册
    msgHandlerMap_.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    msgHandlerMap_.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    msgHandlerMap_.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    msgHandlerMap_.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    msgHandlerMap_.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    msgHandlerMap_.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    msgHandlerMap_.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
    msgHandlerMap_.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});
    // 连接redis服务器
    if (redis_.connect())
    {
        redis_.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}
 
 
// 消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的事件处理回调
    auto it = msgHandlerMap_.find(msgid);
    if (it == msgHandlerMap_.end()) // 找不到 
    {
        // 返回一个默认的处理器，空操作，=按值获取 
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp) {
            LOG_ERROR << "msgid:" << msgid << " can not find handler!"; // muduo日志会自动输出endl 
        };
    }
    else//成功的话 
    {
        return msgHandlerMap_[msgid];   // 返回这个处理器 
    }
}
 
// 处理登录业务  id  pwd   pwd
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO<<"Processing login services！";
    int id = js["id"].get<int>();
    string pwd = js["password"];
    
    User user = userModel_.query(id);
    if (user.getId() == id && user.getPwd() == pwd){
        if (user.getState() == "online")
        {
            // Repeat login is not allowed
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "This account has been logged in, please re-enter the new account";
            conn->send(response.dump());    // 回调函数 返回json字符串
        }
        else
        {
            {
                // 登录成功 记录用户连接
                // 一个用户一个connection，登录成功就可以保存connection
                // 只对连接这块加互斥锁
                lock_guard<mutex> lock(connMutex_);
                userConnMap_.insert(make_pair(id, conn));
            }
            
            // id用户登录成功后，向redis订阅channel(id)
            redis_.subscribe(id);
            
            // Login succeeded. Update the user status
            user.setState("online");
            userModel_.updateState(user);
            
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["errmsg"] = "登录成功！";
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询用户是否有离线消息
            vector<string> vec = offlineMsgModel_.query(id);
            if(!vec.empty())
            {
                response["offlinemsg"] = vec;   // 读取该用户的离线消息后
                offlineMsgModel_.remove(id);    // 删除该用户所有离线消息
            }
            
            // 查询用户的好友信息
            vector<User> userVec = friendModel_.query(id);
            if (!userVec.empty())
            {
                vector<string> vecf;
                for (User &user : userVec)
                {
                    json jsf;
                    jsf["id"] = user.getId();
                    jsf["name"] = user.getName();
                    jsf["state"] = user.getState();
                    vecf.push_back(jsf.dump());
                }
                response["friends"] = vecf;
            }

            // 查询用户的群组信息
            vector<Group> groupuserVec = groupModel_.queryGroups(id);
            if (!groupuserVec.empty())
            {
                vector<string> vecG;
                for (Group &group : groupuserVec)
                {
                    json jsg;
                    jsg["id"] = group.getId();
                    jsg["groupname"] = group.getName();
                    jsg["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json jsu;
                        jsu["id"] = user.getId();
                        jsu["name"] = user.getName();
                        jsu["state"] = user.getState();
                        jsu["role"] = user.getRole();
                        userV.push_back(jsu.dump());
                    }
                    jsg["users"] = userV;
                    vecG.push_back(jsg.dump());
                }
            }
            conn->send(response.dump());
        }
    }
    else
    {
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户名或者密码错误";
        conn->send(response.dump());    // 回调 ，返回json字符串
    }
}
 
// 注册业务  name  password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
	LOG_INFO<<"Handle registration business！";
    string name = js["name"];
    string pwd = js["password"];

    User user;          // Create user objects
    user.setName(name);
    user.setPwd(pwd);
    bool state = userModel_.insert(user);   // Insert a new user
    if (state)  // Inserted successfully
    {
        // registered successfully
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else // Insertion failure
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());    // Callback, returns a json string
    } 
}

// Private chat
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp timr)
{
    int to_id = js["to"].get<int>();
    {
        lock_guard<mutex>lock(connMutex_);
        auto it = userConnMap_.find(to_id);
        if(it != userConnMap_.end())
        {
            // 「to_id」online. Server push message
            it->second->send(js.dump());
            return ;
        }
    }

    // 查询是否在线，如果在线，但前面的map中没有，但是在线，代表在别的服务器中
    User user = userModel_.query(to_id);
    if (user.getState() == "online")
    {
        redis_.publish(to_id, js.dump());
        return;
    }

    // 「to_id」is offline. Processing offline messages
    offlineMsgModel_.insert(to_id, js.dump());
}

// 客户端异常关闭
void ChatService::clientCloseException(const TcpConnectionPtr &conn){
    User user;
    {
        // 以conn从哈希表中倒查主键id
        lock_guard<mutex> lock(connMutex_);
        for (auto it = userConnMap_.begin(); it != userConnMap_.end(); it++)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                userConnMap_.erase(it);
                break;
            }
        }
    }
    // 用户下线了 在redis中取消订阅
    redis_.unsubscribe(user.getId());
    // 更新用户的状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        userModel_.updateState(user);
    }
}

// 客户端用户状态重置
void ChatService::reset(){
    // 把所有online状态的用户设为「offline」
    userModel_.resetState();
}

// 添加好友
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();               //获取用户id
    int friendid = js["friendid"].get<int>();       //获取好友id

    bool state = friendModel_.insert(userid, friendid);
    if(state)
    {
        // 添加好友成功
        json response;
        response["msgid"] = ADD_FRIEND_MSG_ACK;
        response["error"] = 0;
        response["errmsg"]="成功添加好友！";
        response["id"] = userid;
        response["friendid"] = friendid;
        conn->send(response.dump());
    }
    else
    {
        // 添加好友失败
        json response;
        response["msgid"] = ADD_FRIEND_MSG_ACK;
        response["error"] = 1;
        response["errmsg"]="添加好友失败！";
        conn->send(response.dump());
    }
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>(); // 创建群组的用户id
    string name = js["groupname"];    // 群组名称
    string desc = js["groupdesc"];    // 群组描述
 
    // 存储新创建的群组信息
    Group group(-1, name, desc);
    bool state = groupModel_.createGroup(group);
    if (state)
    {
        // 群组创建成功
        json response;
        response["msgid"] = CREATE_GROUP_MSG_ACK;
        response["error"] = 0;
        response["errmsg"] = "Group creation succeeded!";
        response["groupid"] = group.getId();
        response["groupname"] = group.getName();
        response["groupdesc"] = group.getDesc();
 
        // 存储群组创建人信息
        bool pstate = groupModel_.addGroup(userid, group.getId(), "creator");
        if (pstate)
        {
            response["creategroup_userid"] = userid;
        }
        conn->send(response.dump());
    }
    else
    {
        // 群组创建失败
        json response;
        response["msgid"] = CREATE_GROUP_MSG_ACK;
        response["error"] = 1;
        response["errmsg"] = "Group creation failure!";
        conn->send(response.dump());
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();       // 想加入群组的用户id
    int groupid = js["groupid"].get<int>(); // 群组id号
    bool pstate = groupModel_.addGroup(userid, groupid, "normal");
    if (pstate)
    {
        json response;
        response["msgid"] = ADD_GROUP_MSG_ACK;
        response["error"] = 0;
        response["errmsg"] = "Successfully join a group!";
        response["userid"] = userid;
        response["groupid"] = groupid;
        conn->send(response.dump());
    }
    else
    {
        json response;
        response["msgid"] = ADD_GROUP_MSG_ACK;
        response["error"] = 1;
        response["errmsg"] = "Failed to join a group!";
        conn->send(response.dump());
    }
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = groupModel_.queryGroupUsers(userid, groupid); // 查询这个用户所在群组的其他用户id
 
    lock_guard<mutex> lock(connMutex_); // 不允许其他人在map里面增删改查
    for (int id : useridVec)
    {
        auto it = userConnMap_.find(id);
        if (it != userConnMap_.end())   // 证明在线，直接转发
        {
            // 转发群消息
            it->second->send(js.dump());
        }
        else // 离线
        {
            User user = userModel_.query(id);
            if (user.getState() == "online")
            {
                redis_.publish(id, js.dump());
            }
            else{
                // 存储离线群消息
                offlineMsgModel_.insert(id, js.dump());
            }
        }
    }
}

// 注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(connMutex_);
        auto it = userConnMap_.find(userid);
        if (it != userConnMap_.end())
        {
            userConnMap_.erase(it);
        }
    }
    redis_.unsubscribe(userid);
 
    // 更新用户状态信息         
    User user(userid, "", "", "offline");
    userModel_.updateState(user);
}

// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    // 用户在线
    lock_guard<mutex> lock(connMutex_);
    auto it = userConnMap_.find(userid);
    if (it != userConnMap_.end())
    {
        it->second->send(msg);
        return;
    }
    offlineMsgModel_.insert(userid, msg);
}

