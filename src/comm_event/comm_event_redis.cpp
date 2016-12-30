/**
 **/

#include "comm_event_redis.h"
#include <hiredis.h>
#include <async.h>
#include <adapters/libevent.h>
#include <cstdarg>
#include <iostream>

namespace dc
{
namespace common
{
namespace comm_event
{

std::map<const redisAsyncContext *, CommRedis *> CommRedis::map_inst_;

CommRedis::~CommRedis()
{
    if(ac_ != NULL)
    {
        if(map_inst_.count(ac_)>0)
        {
            map_inst_.erase(ac_);
        }
//disconnect之后，会自动释放ac_, 所以没有这个判断，会导致double free
        if(status_ != DISCONNECTED)
        {
            ::redisAsyncFree(ac_);
        }
        ac_=NULL;
    }
}

int CommRedis::reconnect()
{
    int rt=0;
    do{
        if(host_.empty() || port_ <= 0)
        {
            rt =-1;
            break;
        }
        if(status_ != DISCONNECTED)
        {
            rt=-2;
            break;
        }
        if(map_inst_.count(ac_) > 0)
        {
            map_inst_.erase(ac_);
            ac_=NULL;
        }
        status_=UNCONNECTED;
        rt=connect(host_, port_);

    }while(0);
    return rt;
}

int CommRedis::connect(const std::string & host, int port)
{
    int rt=0;
    do{
        host_=host;
        port_=port;
        ac_=::redisAsyncConnect(host.c_str(), port);
        if(ac_ == NULL)
        {
            rt=-1;
            break;
        }
        if(ac_->err != 0)
        {
            rt=-2;
            break;
        }
        status_=CONNECTING;
        map_inst_[ac_]=this;
    }while(0);
    if(rt != 0)
    {
        if(ac_ != NULL)
        {
            redisAsyncFree(ac_);
            ac_=NULL;
        }
    }
    return rt;
}

void CommRedis::disconnect()
{
    //将导致ac_被释放
    ::redisAsyncDisconnect(ac_);
}

int CommRedis::send_cmd(void * sess_data, const char * fmt, ...)
{
    int rt=0;
    do{
        if(ac_ == NULL)
        {
            rt=-1;
            break;
        }
        va_list ap;
        va_start(ap, fmt);
        int status=redisvAsyncCommand(ac_, cmd_callback, sess_data, fmt, ap);
        va_end(ap);
        if(status != REDIS_OK)
        {
            rt=-2;
            break;
        }

    }while(0);
    return rt;
}

int CommRedis::handle_get_result(void * sess_data, void * pRedisReply)
{
    return 0;
}

/**
 * @brief               链接完成时被调用
 **/
void CommRedis::on_connected()
{
    return;
}

/**
 * @brief               链接断开时被
 **/
void CommRedis::on_disconnect()
{
    return;
}


int CommRedis::add_to_base(struct event_base * base)
{
    int rt=0;
    do{
        if(ac_ == NULL)
        {
            rt=-1;
            break;
        }
        ::redisLibeventAttach(ac_, base);
        ::redisAsyncSetConnectCallback(ac_, connect_callback);
        ::redisAsyncSetDisconnectCallback(ac_, disconnect_callback );

    }while(0);
    return rt;
 
}

void CommRedis::cmd_callback(redisAsyncContext *c, void *r, void *privdata)
{
    CommRedis * cr=map_inst_[c];
    if(cr != NULL)
    {
        cr->handle_get_result(privdata, r);
    }
}


void CommRedis::connect_callback(const redisAsyncContext *c, int status)
{
    CommRedis * cr=map_inst_[c];
    if(cr != NULL)
    {
        if(status!=REDIS_OK)
        {
            cr->on_error("connect_error");
        }else
        {
            cr->status_=CONNECTED;
            cr->on_connected();
        }
    }
}

void CommRedis::on_error(const char * errstr)
{
    std::cerr<<"CommRedis on error("<<errstr<<")"<<std::endl;
}


void CommRedis::disconnect_callback(const redisAsyncContext *c, int status)
{
    CommRedis * cr=map_inst_[c];
    if(cr != NULL)
    {
        cr->status_=DISCONNECTED;
        cr->on_disconnect();
    }
}

}
}  
}



