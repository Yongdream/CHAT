#include "redis.hpp"
#include <iostream>

using namespace std;

Redis::Redis()
    : publishContext_(nullptr)
    , subcribeContext_(nullptr)
    {}

Redis::~Redis()
{
    if (publishContext_ != nullptr)
    {
        redisFree(publishContext_);
    }

    if (subcribeContext_ != nullptr)
    {
        redisFree(subcribeContext_);
    }
}

// 连接redis服务器
bool Redis::connect()
{
    // 负责发布消息的上下文连接
    publishContext_ = redisConnect("192.168.122.129", 6379);
    if (nullptr == publishContext_)
    {
        cerr << "Connect redis failed!" << endl;
        return false;
    }

    // 负责订阅消息的上下文连接
    subcribeContext_ = redisConnect("192.168.122.129", 6379);
    if (nullptr == subcribeContext_)
    {
        cerr << "Connect redis failed!" << endl;
        return false;
    }

    // 在单独的线程中，监听通道上的事件，有消息给业务层进行上报
    /**
     * 创建一个新的线程对象 t
     * thread 构造函数接受一个可调用对象作为参数
     * 例子中接受的是一个 lambda 表达式
     * */
    thread t([&]()   
    {
        observer_channel_message();
    });
    t.detach(); // 分离线程

    cout << "connect redis-server success!" << endl;

    return true;
}

// 向redis指定的通道channel发布消息
bool Redis::publish(int channel, string message)
{
    /**
     * 发布一条消息到一个指定的频道
     * redisReply* redisCommand(redisContext *c, const char *format, ...);
    */
    redisReply *reply = (redisReply *)redisCommand(publishContext_, 
                                                    "PUBLISH %d %s", 
                                                    channel, 
                                                    message.c_str());
    if (nullptr == reply)
    {
        cerr << "publish command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel)
{
    /**
     * SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息，
     * 只订阅通道，不接收通道消息
     * 通道消息的接收专门在 observer_channel_message 函数中的独立线程中进行
     * 只负责发送命令，不阻塞接收redis server响应消息，否则和 notifyMsg 线程抢占响应资源
     * int redisAppendCommand(redisContext *c, const char *format, ...);
    */
    if (REDIS_ERR == redisAppendCommand(this->subcribeContext_, "SUBSCRIBE %d", channel))
    {
            cerr << "subscribe command failed!" << endl;
            return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
            if (REDIS_ERR == redisBufferWrite(this->subcribeContext_, &done))
            {
                cerr << "subscribe command failed!" << endl;
                return false;
            }
    }
    // redisGetReply

    return true;
}

// 向redis指定的通道unsubscribe取消订阅消息
bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(this->subcribeContext_, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }

    int done = 0;
    while (!done)
    {
            if (REDIS_ERR == redisBufferWrite(this->subcribeContext_, &done))
            {
                cerr << "subscribe command failed!" << endl;
                return false;
            }
    }

    return true;
}

// 在独立线程中接受订阅通道的消息
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    /**
     * 从 Redis 服务器获取订阅消息
     * int redisGetReply(redisContext *c, void **reply);
     * void **reply 是指向存储回复的指针的指针
     * 订阅收到的消息是一个带三元素的数组:
     * 1. 消息类型（如 "message"）
     * 2. 频道名称
     * 3. 消息内容
    */
    while (REDIS_OK == redisGetReply(this->subcribeContext_, (void **)&reply))
    {
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 给业务层上报通道发生的消息
            notifyMessageHandler_(atoi(reply->element[1]->str), reply->element[2]->str);
        }

        freeReplyObject(reply);
    }
    cerr << ">>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<" << endl;
}

// 初始化向业务层上报通道的回调对象
void Redis::init_notify_handler(NotifyHandler fn)
{
    this->notifyMessageHandler_ = fn;
}