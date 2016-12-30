/**
 * @company             
 */

#ifndef  COMM_EVENT_COMM_EVENT_FACTORY_H_
#define  COMM_EVENT_COMM_EVENT_FACTORY_H_

#include <iostream>

extern "C"
{
#include "event2/event.h"
}

#include "comm_event_interface.h"

namespace dc
{
namespace common
{
namespace comm_event
{

/** 
 * @class CommFactory
 * @brief CommFactory          
 **/
class CommFactory:public CommFactoryInterface
{
public:

    /** 
     * @brief               进行基本的初始化
     * @return              成功返回EC_SUCCESS, 否则返回对应的错误码
     **/
    virtual int init();
 

    /**
     * @brief               fork之后, 由子进程调用必须调用
     * @return              成功返回0, 否则返回-1
     */
    virtual int re_init();

    /** 
     * @brief               添加一个comm实例
     * @param comm          [IN],通讯类
     * @return              成功返回EC_SUCCESS, 否则返回对应的错误码
     **/
    virtual int add_comm(CommUdpInterface * comm);

    virtual int add_comm_tcp(CommTcpInterface * tcpcomm);

    virtual int add_comm_timer(CommTimerInterface * timercomm);

    virtual int add_comm_redis(CommRedisInterface * rediscomm);

    /** 
     * @brief               开始跑
     * @return              成功返回EC_SUCCESS, 否则返回对应的错误码
     **/
    virtual int run();

private:
    struct event_base * base_;

};



}//comm_event
};//common
};//dc

#endif   /* ----- #ifndef COMM_EVENT_COMM_EVENT_FACTORY_H_  ----- */

