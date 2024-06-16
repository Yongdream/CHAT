#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h> // muduo的日志 
#include <string>

using namespace std;
using namespace muduo;
 
//获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}
 
//构造方法，注册消息以及对应的Handler回调操作
ChatService::ChatService()
{
    //用户基本业务管理相关事件处理回调注册
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});

}
 
 
//获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    //记录错误日志，msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end()) // 找不到 
    {
        //返回一个默认的处理器，空操作，=按值获取 
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp) {
            LOG_ERROR << "msgid:" << msgid << " can not find handler!"; // muduo日志会自动输出endl 
        };
    }
    else//成功的话 
    {
        return _msgHandlerMap[msgid];//返回这个处理器 
    }
}
 
// 处理登录业务  id  pwd   pwd
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO<<"Processing login services！";
    int id = js["id"].get<int>();
    string pwd = js["password"];
    
    User user = _userModel.query(id);
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
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert(make_pair(id, conn));
            }

            // Login succeeded. Update the user status
            user.setState("online");
            _userModel.updateState(user);
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["errmsg"] = "登录成功！";
            response["id"] = user.getId();
            response["name"] = user.getName();
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
 
//处理注册业务  name  password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
	LOG_INFO<<"Handle registration business！";
    string name = js["name"];
    string pwd = js["password"];

    User user;          // Create user objects
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);   // Insert a new user
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