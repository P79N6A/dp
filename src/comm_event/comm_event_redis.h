/**
 **/

#ifndef  _COMM_EVENT_REDIS_H_ 
#define  _COMM_EVENT_REDIS_H_

#include <stdlib.h>
#include <iostream>

extern "C"
{
#include "event2/event.h"
#include <hiredis.h>
#include <async.h>
#include <adapters/libevent.h>  //hiredis libevent adapter
}
#include "comm_event_interface.h"
#include <map>

namespace dc
{
namespace common
{
namespace comm_event
{

/*Í¨Ñ¶µÄ¹¤³§Àà*/
class CommFactory;


/** 
 * @brief CommRedis
 **/
class CommRedis:public CommRedisInterface
{
public:
    enum
    {
        UNCONNECTED=0,
        CONNECTING,
        CONNECTED,
        DISCONNECTED,
    };

    CommRedis():ac_(NULL),status_(UNCONNECTED){}
    
    virtual ~CommRedis();

    virtual int connect(const std::string & host, int port);

    virtual int reconnect();

    virtual void disconnect();

    /**
     * @brief               发送命令
     * @param sess_data     [IN], 发送的sess数据，最终会被handle_get_result获取
     **/
    int send_cmd(void * sess_data, const char * fmt, ...);


    virtual int handle_get_result(void * sess_data, void * pRedisReply);


    /**
     * @brief               链接完成时被调用
     **/
    virtual void on_connected();


    /**
     * @brief               链接断开时被
     **/
    virtual void on_disconnect();

    virtual void on_error(const char * errstr);

    /** 
     * @brief               把自己加到base里面去               
     * @param base          [IN]
     * @return              0-success, other-fail
     **/
    int add_to_base(struct event_base * base);


    /**
     * @brief               执行命令的callback
     **/
    static void cmd_callback(redisAsyncContext *c, void *r, void *privdata);

    /**
     * @brief               执行命令的callback
     **/
    static void connect_callback(const redisAsyncContext *c, int status);


    static void disconnect_callback(const redisAsyncContext *c, int status);


    /**
     * @brief               根据ac获取RedisComm实例
     * @return              NULL if fail
     **/
    static CommRedis * get_instance(redisAsyncContext * ac)
    {
        if(map_inst_.count(ac) > 0)
        {
            return map_inst_[ac];
        }else
        {
            return NULL;
        }
    }

    int err_code()
    {
        if(ac_ != NULL)
        {
            return ac_->err;
        }else
        {
            return -999;
        }
    }

    const char * err_str()
    {
        if(ac_!=NULL && ac_->err != 0 )
        {
            return ac_->errstr;
        }else
        {
            return "no error";
        }
    }

private:
    redisAsyncContext * ac_;    //异步上下文
    std::string host_;          //主机
    int port_;                  //端口
    int status_;

    //ac-->RedisComm
    static std::map<const redisAsyncContext *, CommRedis *> map_inst_;
};

}//comm_event
};//common
};//dc
#endif   // ----- #ifndef _COMM_EVENT_REDIS_H_  ----- 
