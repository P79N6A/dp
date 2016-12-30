#ifndef H_LIBMUDUOBRIDGE
#define H_LIBMUDUOBRIDGE
/*
 * Copyright (c) 2016/11/19, zhangxianghui
 *
 */
#include <muduo/net/Channel.h>
#include <muduo/net/EventLoop.h>
#include <async.h>



class RedisChannelBridge;

typedef boost::function<void(redisReply *reply, void *priv)> OnRedisReply;
typedef boost::function<void()> OnRedisChannelDisconnect;

class RedisChannelBridge
{
public:
    explicit RedisChannelBridge(muduo::net::EventLoop*);
    ~RedisChannelBridge();
    int Connect(const std::string &ip, int port);
    bool Connected();
    void Disconnect();
    muduo::net::EventLoop* GetLoop()
    {
        return loop_;
    }
    //0 success -1 failed!
    int RedisCommand(void *priv, const std::string &cmd);
    int RedisCommand(void *priv, const char *cmd, int len);
    int RedisCommand(void *priv, const char *format, ...);
    //redis reply callback
    void SetRedisReplyCb(const OnRedisReply &cb)
    {
        reply_cb_ = cb;
    }
    //on redis disconnect event
    void SetRedisDisconnectCb(const OnRedisChannelDisconnect &cb)
    {
        on_disconnect_ = cb;
    }
private:
    typedef boost::function<void(redisAsyncContext*, void*, void*)> SelfRedisCallBack;
    struct ContextData {
        muduo::net::Channel *chn;
    };
    struct CallBackData {
        RedisChannelBridge *instance;
        void *priv;
    };
    OnRedisReply reply_cb_;
    OnRedisChannelDisconnect on_disconnect_;//redis disconnect event
    muduo::net::EventLoop *loop_;
    bool connected;
    redisAsyncContext *ctx_;
    void OnRead(muduo::Timestamp);
    void OnWrite();
    void redisMuduoAttach(redisAsyncContext *c);
    static void FreeCtx(ContextData *cd);
    //disable copy && assign
    RedisChannelBridge(const RedisChannelBridge &rhs);
    RedisChannelBridge& operator = (const RedisChannelBridge &rhs);

    static void OnredisConnected(const struct redisAsyncContext*, int status);
    static void OnredisDisConnected(const struct redisAsyncContext*, int status);
    static void OnRedisCallBack(struct redisAsyncContext*, void*, void*);
    static void redisMuduoAddRead(void *privdata);
    static void redisMuduoDelRead(void *privdata);
    static void redisMuduoAddWrite(void *privdata);
    static void redisMuduoDelWrite(void *privdata);
    static void redisMuduoCleanup(void *privdata);
};

#endif
