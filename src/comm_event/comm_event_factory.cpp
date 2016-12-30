/**
 * @company             
 */

//include self head file
#include"comm_event_factory.h"

#include"comm_event.h"
#include"comm_event_tcp.h"
#include"comm_event_timer.h"
#include"comm_event_redis.h"

//include STD C head files
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//include STD C++ head files


//include third_party_lib head files


//include project other head files

namespace dc 
{

namespace common
{
namespace comm_event
{

/** 
 * @brief               ���л����ĳ�ʼ��
 * @param
 * @return              �ɹ�����EC_SUCCESS, ���򷵻ض�Ӧ�Ĵ�����
 **/
int CommFactory::init()
{
    base_=event_base_new();
    if(base_==NULL)
    {
        return EC_NEW_BASE_ERROR;
    }
    return EC_SUCCESS;
}

int CommFactory::re_init()
{
	return event_reinit(base_);
}

/** 
 * @brief               ���һ��commʵ��
 * @param comm          [IN],ͨѶ��
 * @return              �ɹ�����EC_SUCCESS, ���򷵻ض�Ӧ�Ĵ�����
 **/
int CommFactory::add_comm(CommUdpInterface * comm)
{
    int rt=EC_SUCCESS;
    do{
        if(comm==NULL)
        {
            rt=EC_PARAM_ERROR;
            break;
        }
        CommBase * commbase = dynamic_cast<CommBase *>(comm);
        rt=commbase->add_to_base(base_);
        if(rt != EC_SUCCESS)
        {
            break;
        }

    }while(0);
    return rt;
}

int CommFactory::add_comm_tcp(CommTcpInterface * tcpcomm)
{
    int rt=EC_SUCCESS;
    do{
        if(tcpcomm==NULL)
        {
            rt=EC_PARAM_ERROR;
            break;
        }
        CommTcpBase * commbase = dynamic_cast<CommTcpBase *>(tcpcomm);
        rt=commbase->add_to_base(base_);
        if(rt != EC_SUCCESS)
        {
            break;
        }
    }while(0);
    return rt;

}

int CommFactory::add_comm_timer(CommTimerInterface * timercomm)
{
	int rt=EC_SUCCESS;
	do{
		if(timercomm==NULL)
        {
        	rt=EC_PARAM_ERROR;
        	break;
        }
        TimerBase * sess=dynamic_cast<TimerBase *>(timercomm);
        rt=sess->add_to_base(base_);
        if(rt != EC_SUCCESS)
        {
        	break;
        }
	}while(0);
	return rt;
}

int CommFactory::add_comm_redis(CommRedisInterface * rediscomm)
{
	int rt=EC_SUCCESS;
	do{
		if(rediscomm==NULL)
        {
        	rt=EC_PARAM_ERROR;
        	break;
        }
        CommRedis* redis=dynamic_cast<CommRedis*>(rediscomm);
        rt=redis->add_to_base(base_);
        if(rt != EC_SUCCESS)
        {
        	break;
        }
	}while(0);
	return rt;
}

/** 
 * @brief               ��ʼ��
 * @return              �ɹ�����EC_SUCCESS, ���򷵻ض�Ӧ�Ĵ�����
 **/
int CommFactory::run()
{
    event_base_dispatch(base_);
    return -1;
}

CommFactoryInterface & CommFactoryInterface::instance()
{
    static CommFactory factory;
    return factory;
}

}//comm_event
}//common
}//dc
