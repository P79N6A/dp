#ifndef REDISCLIENT_H
#define REDISCLIENT_H

#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <muduo/net/Buffer.h>
#include "libmuduobridge.h"

/*
auth: zhangxh
date: 2016/11/29

*/
namespace poseidon
{
namespace adapter
{

class muduo::net::EventLoop;
class RedisClient : public boost::noncopyable
{
public:
    explicit RedisClient(muduo::net::EventLoop*);
    virtual ~RedisClient();
    void Command(const std::string &cmd);
    void Command(const char *, int);
protected:
    RedisChannelBridge bridge_;
private:
    std::string server_;
    int port_;
    boost::function<void()> Reconnected_cb_;
    void OnRedisDisconnected();
    void OnRedisReply(redisReply *reply);
    void ParseReply(redisReply **element, size_t num);
    void Reconnect();
};

}

}

#endif // REDISCLIENT_H
