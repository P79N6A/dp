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
     * @brief               ���л����ĳ�ʼ��
     * @return              �ɹ�����EC_SUCCESS, ���򷵻ض�Ӧ�Ĵ�����
     **/
    virtual int init();
 

    /**
     * @brief               fork֮��, ���ӽ��̵��ñ������
     * @return              �ɹ�����0, ���򷵻�-1
     */
    virtual int re_init();

    /** 
     * @brief               ���һ��commʵ��
     * @param comm          [IN],ͨѶ��
     * @return              �ɹ�����EC_SUCCESS, ���򷵻ض�Ӧ�Ĵ�����
     **/
    virtual int add_comm(CommUdpInterface * comm);

    virtual int add_comm_tcp(CommTcpInterface * tcpcomm);

    virtual int add_comm_timer(CommTimerInterface * timercomm);

    virtual int add_comm_redis(CommRedisInterface * rediscomm);

    /** 
     * @brief               ��ʼ��
     * @return              �ɹ�����EC_SUCCESS, ���򷵻ض�Ӧ�Ĵ�����
     **/
    virtual int run();

private:
    struct event_base * base_;

};



}//comm_event
};//common
};//dc

#endif   /* ----- #ifndef COMM_EVENT_COMM_EVENT_FACTORY_H_  ----- */

