#include "libmuduobridge.h"

#include <boost/bind.hpp>
#include <muduo/base/Logging.h>
/*
auth:zhangxianghui
date:2016/11/28
redis-muduo lib bridge
*/


RedisChannelBridge::RedisChannelBridge(muduo::net::EventLoop *loop) : loop_(loop),
    connected(false)
{

}

RedisChannelBridge::~RedisChannelBridge()
{

}

int RedisChannelBridge::Connect(const std::string &ip, int port)
{
    if(!Connected()) {
        LOG_INFO << "Connect to redis:[" << ip << ":" << port << "]";
        //ctx生命周期我们不管理，由Async.c执行释放
        ctx_ = redisAsyncConnect(ip.c_str(), port);
        if(!ctx_) {
            return -1;
        }
        redisAsyncSetConnectCallback(ctx_, &RedisChannelBridge::OnredisConnected);
        redisAsyncSetDisconnectCallback(ctx_, &RedisChannelBridge::OnredisDisConnected);

        struct ContextData *cd = new ContextData();
        cd->chn = new muduo::net::Channel(loop_, ctx_->c.fd);
        cd->chn->setReadCallback(boost::bind(&RedisChannelBridge::OnRead, this, _1));
        cd->chn->setWriteCallback(boost::bind(&RedisChannelBridge::OnWrite, this));
        cd->chn->enableReading();
        cd->chn->enableWriting();
        ctx_->data = cd;
        redisMuduoAttach(ctx_);
        connected = true;
        return 0;
    } else {
        return -1;
    }
}

bool RedisChannelBridge::Connected()
{
    return connected;
}

void RedisChannelBridge::Disconnect()
{
    if (ctx_ && !(ctx_->c.flags & REDIS_DISCONNECTING)) {
        redisAsyncDisconnect(ctx_);
    }
}


void RedisChannelBridge::OnredisConnected(const struct redisAsyncContext *c, int status)
{
    LOG_INFO << "On redis connected event";
    struct ContextData *cd = (struct ContextData*)c->data;
    muduo::net::Channel *chnl = (muduo::net::Channel *)cd->chn;
    (void)chnl;
}

void RedisChannelBridge::OnredisDisConnected(const struct redisAsyncContext *c, int status)
{
    LOG_INFO << "On redis disConnected event";
}
//muduo event
void RedisChannelBridge::OnRead(muduo::Timestamp)
{
    //printf("onread\n");
    struct ContextData *cd = (struct ContextData*)ctx_->data;
    muduo::net::Channel *chnl = (muduo::net::Channel *)cd->chn;
    //若channel已经被其他线程禁止监听任何事件，那么在loop中对剩余的读写事件不再处理！
    //必须这样，否则CoreDump
    if(!chnl->isNoneEvent()) {
        redisAsyncHandleRead(ctx_);
    }
}

//muduo event
void RedisChannelBridge::OnWrite()
{
    //printf("onwrite\n");
    struct ContextData *cd = (struct ContextData*)ctx_->data;
    muduo::net::Channel *chnl = (muduo::net::Channel *)cd->chn;
    if(!chnl->isNoneEvent()) {
        redisAsyncHandleWrite(ctx_);
    }
}

void RedisChannelBridge::OnRedisCallBack(struct redisAsyncContext *c, void *reply, void *privatedata)
{
    LOG_DEBUG << "OnRedisCallBack";
    CallBackData *cb = (CallBackData*)privatedata;
    RedisChannelBridge *instance = cb->instance;
    if(reply && instance->reply_cb_) {
        instance->reply_cb_((redisReply *)reply, cb->priv);
    }
    delete cb;
}

int RedisChannelBridge::RedisCommand(void *priv, const char *cmd, int len)
{
    if(!connected) {
        return -1;
    }
    struct ContextData *cd = (struct ContextData*)ctx_->data;
    muduo::net::Channel *chnl = (muduo::net::Channel *)cd->chn;
    if(!(ctx_->c.flags & REDIS_CONNECTED) || chnl->isNoneEvent()) {
        return -1;
    }
    CallBackData *cb = new CallBackData();
    cb->instance = this;
    cb->priv = priv;
    return redisAsyncCommand(ctx_, &RedisChannelBridge::OnRedisCallBack, cb, cmd, len);
}

int RedisChannelBridge::RedisCommand(void *priv, const char *format, ...)
{
    if(!connected) {
        return -1;
    }
    struct ContextData *cd = (struct ContextData*)ctx_->data;
    muduo::net::Channel *chnl = (muduo::net::Channel *)cd->chn;
    if(!(ctx_->c.flags & REDIS_CONNECTED) || chnl->isNoneEvent()) {
        return -1;
    }
    CallBackData *cb = new CallBackData();
    cb->instance = this;
    cb->priv = priv;
    va_list ap;
    int status;
    va_start(ap,format);
    status = redisvAsyncCommand(ctx_, &RedisChannelBridge::OnRedisCallBack, cb, format,ap);
    va_end(ap);
    return status;
}

int RedisChannelBridge::RedisCommand(void *priv, const std::string &cmd)
{
    return RedisCommand(priv, cmd.c_str(), cmd.length());
}

void RedisChannelBridge::redisMuduoAddRead(void *privdata)
{
    //printf("redisMuduoAddRead\n");
    RedisChannelBridge *instance = (RedisChannelBridge*)privdata;
    struct ContextData *cd = (struct ContextData*)instance->ctx_->data;
    muduo::net::Channel *chnl = (muduo::net::Channel *)cd->chn;
    //chnl->enableReading();

}

void RedisChannelBridge::redisMuduoDelRead(void *privdata)
{
    //printf("redisMuduoDelRead\n");
    RedisChannelBridge *instance = (RedisChannelBridge*)privdata;
    struct ContextData *cd = (struct ContextData*)instance->ctx_->data;
    muduo::net::Channel *chnl = (muduo::net::Channel *)cd->chn;
    //chnl->disableReading();
}

void RedisChannelBridge::redisMuduoAddWrite(void *privdata)
{
    //printf("redisMuduoAddWrite\n");
    RedisChannelBridge *instance = (RedisChannelBridge*)privdata;
    struct ContextData *cd = (struct ContextData*)instance->ctx_->data;
    muduo::net::Channel *chnl = (muduo::net::Channel *)cd->chn;
    chnl->enableWriting();
}

void RedisChannelBridge::redisMuduoDelWrite(void *privdata)
{
    //printf("redisMuduoDelWrite\n");
    RedisChannelBridge *instance = (RedisChannelBridge*)privdata;
    struct ContextData *cd = (struct ContextData*)instance->ctx_->data;
    muduo::net::Channel *chnl = (muduo::net::Channel *)cd->chn;
    chnl->disableWriting();
}


//请求清除环境，通常意味着连接已经断开
void RedisChannelBridge::redisMuduoCleanup(void *privdata)
{
    LOG_INFO << "On redis mudule cleanup";
    RedisChannelBridge *instance = (RedisChannelBridge*)privdata;
    struct ContextData *cd = (struct ContextData*)instance->ctx_->data;
    muduo::net::Channel *chnl = cd->chn;
    chnl->disableAll();
    chnl->remove();
    instance->connected = false;
    //由于muduocleanup肯定是在onread或onwrite触发的，因此释放channel需要
    //在queuninloop中执行，以免channel::~channel中的asset(eventHandling_)触发
    //void (RedisCtxPtr::*ctx_reset)();
    //ctx_reset = &RedisCtxPtr::reset;
    //延长ctx_生命周期，避免在connect中被释放
    //boost::function<void()> fn = boost::bind(ctx_reset, ctx);
    //chnl->ownerLoop()->queueInLoop(fn);
    boost::function<void()> fn = boost::bind(&RedisChannelBridge::FreeCtx,
                                 cd);
    instance->loop_->queueInLoop(fn);
    if (instance->on_disconnect_) {
        instance->loop_->queueInLoop(instance->on_disconnect_);
    }
}

void RedisChannelBridge::FreeCtx(ContextData *cd)
{
    LOG_INFO << "free redis channel...";
    muduo::net::Channel *chnl = cd->chn;
    delete chnl;//must deleted outside io event.
    delete cd;
}

void RedisChannelBridge::redisMuduoAttach(redisAsyncContext *c)
{
    /* Register functions to start/stop listening for events */
    c->ev.addRead = redisMuduoAddRead;
    c->ev.delRead = redisMuduoDelRead;
    c->ev.addWrite = redisMuduoAddWrite;
    c->ev.delWrite = redisMuduoDelWrite;
    c->ev.cleanup = redisMuduoCleanup;
    c->ev.data = this;//privdata
}
