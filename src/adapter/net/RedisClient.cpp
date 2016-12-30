#include "RedisClient.h"
#include <boost/bind.hpp>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include "conf/Configer.h"

extern poseidon::adapter::Configer * volatile configer;

namespace poseidon
{
namespace adapter
{

namespace
{
unsigned int kReconnect_interval = 10;
}

RedisClient::RedisClient(muduo::net::EventLoop *loop) : bridge_(loop)
{
    //ctor
    server_ = configer->GetProperty<std::string>("redis.server", "");
    port_ = configer->GetProperty<int>("redis.port", 6379);
    //LOG_INFO << "Redis server:[" << server_ << "] Port:[" << port_ << "]";
    Reconnected_cb_ = boost::bind(&RedisClient::Reconnect, this);
    bridge_.SetRedisDisconnectCb(boost::bind(&RedisClient::OnRedisDisconnected, this));
    bridge_.SetRedisReplyCb(boost::bind(&RedisClient::OnRedisReply, this, _1));
    bridge_.Connect(server_, port_);
}

RedisClient::~RedisClient()
{
    //dtor
}

void RedisClient::Command(const char *cmd, int len)
{
    if (bridge_.RedisCommand(NULL, cmd, len) != 0) {
        LOG_ERROR << "Exec Redis Command failed.";
    }
}

void RedisClient::Command(const std::string &cmd)
{
    Command(cmd.c_str(), cmd.length());
}

void RedisClient::Reconnect()
{
    LOG_INFO << "Reconnecte to redis!";
    bridge_.Connect(server_, port_);
    if (!(bridge_.Connected())) {
        bridge_.GetLoop()->runAfter((kReconnect_interval *= 2), Reconnected_cb_);
    }
}

void RedisClient::OnRedisDisconnected()
{
    bridge_.GetLoop()->runAfter((kReconnect_interval = 10), /*10s after*/ Reconnected_cb_);//reconnect!
}

void RedisClient::OnRedisReply(redisReply *reply)
{
    if (reply) {
        ParseReply(&reply, 1);
    }
}

void RedisClient::ParseReply(redisReply **element, size_t num)
{
    for(size_t i = 0; i < num; ++i) {
        redisReply *ele = element[i];
        switch(ele->type) {
        case REDIS_REPLY_STRING:
            //printf("REDIS_REPLY_STRING:%s\n", ele->str);
            //LOG_INFO << "REDIS_REPLY_STRING:" << ele->str;
            break;
        case REDIS_REPLY_INTEGER:
            //printf("REDIS_REPLY_INTEGER:%lld\n", ele->integer);
            //LOG_INFO << "REDIS_REPLY_INTEGER:" << ele->integer;
            break;
        case REDIS_REPLY_STATUS:
            //printf("REDIS_REPLY_STATUS:%s\n", ele->str);
            //LOG_INFO << "REDIS_REPLY_STATUS:" << ele->str;
            break;
        case REDIS_REPLY_ERROR:
            //printf("REDIS_REPLY_ERROR:%s\n", ele->str);
            //LOG_INFO << "REDIS_REPLY_ERROR:" << ele->str;
            break;
        case REDIS_REPLY_NIL:
            //printf("REDIS_REPLY_NIL\n");
            //LOG_INFO << "REDIS_REPLY_NIL";
            break;
        case REDIS_REPLY_ARRAY:
            //printf("REDIS_REPLY_ARRAY\n");
            ParseReply(ele->element, ele->elements);
            break;
        default:
            LOG_WARN << "unknow redis reply";
            break;
        }
    }
}

}
}
