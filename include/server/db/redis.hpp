#pragma

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>

using namespace std;

class Redis
{
public:

    // 向业务层上报回调
    using NotifyHandler = function<void(int, string)>;

    Redis();
    ~Redis();

    // 连接redis服务器
    bool connect();

    // 向redis指定的通道channel发布消息
    bool publish(int channel, string message);

    // 向redis指定的通道subscribe订阅消息
    bool subscribe(int channel);

    // 向redis指定的通道unsubscribe取消订阅消息
    bool unsubscribe(int channel);

    // 在独立线程中接受订阅通道的消息
    void observer_channel_message();

    // 初始化向业务层上报通道的回调对象
    void init_notify_handler(NotifyHandler fn);

private:
    // hiredis 同步上下文对象，负责publish消息
    redisContext *publishContext_;

    // hiredis 同步上下文对象，负责subscribe消息
    redisContext *subcribeContext_;

    // 回调 收到订阅消息，向service层上报
    NotifyHandler notifyMessageHandler_;
};

