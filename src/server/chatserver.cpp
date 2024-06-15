#include"chatservice.hpp"
#include"public.hpp"
#include<string>
#include<muduo/base/Logging.h>

using namespace std;
using namespace muduo;

ChatService* ChatService::instance(){
    static ChatService service;

    return &service;
}
    
//注册消息以及对应的回调操作
ChatService::ChatService(){
    _msgHanderMap.insert({LOGIN_TYPE,std::bind(&ChatService::login,this,_1,_2,_3)});
    _msgHanderMap.insert({REG_TYPE,std::bind(&ChatService::reg,this,_1,_2,_3)});
}

//获取存储消息id和对应的处理方法
MsgHandler ChatService::getHandle(int msgid){

    //日志记录
    auto it = _msgHanderMap.find(msgid);
    if(it == _msgHanderMap.end()){
        //返回一个lambda表达式，返回一个默认的空处理器，防止业务挂掉，后可做平滑升级处理        
        return [=](const TcpConnectionPtr &conn,json &js,Timestamp time){
            LOG_ERROR<<"msgid:"<<msgid<<"can not find handle!";
        };
    }
    else{
        return _msgHanderMap[msgid];
    }
}

void ChatService::login(const TcpConnectionPtr &conn,json &js,Timestamp time){
    LOG_INFO<<"login";
}

void ChatService::reg(const TcpConnectionPtr &conn,json &js,Timestamp time){
    LOG_INFO<<"regist";
}

