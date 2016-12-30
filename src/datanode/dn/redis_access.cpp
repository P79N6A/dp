/**
 **/

#include "redis_access.h"
#include "comm_event_redis.h"
#include "comm_event_interface.h"
#include "comm_event_factory.h"
#include "monitor_api.h"
#include "attr.h"
#include <algorithm>

namespace poseidon
{
namespace dn
{

int RedisAccess::handle_get_result(void * sess_data, void * pRedisReply)
{
    std::list<handle_func *>::iterator it;
    for(it=func_list_.begin(); it!=func_list_.end(); it++)
    {
        (*it)(sess_data, pRedisReply, err_code(), err_str());
    }
    return 0;
}

/**
 * @brief               链接完成时被调用
 **/
void RedisAccess::on_connected()
{
    MON_ADD(ATTR_REDIS_ON_CONNECTED, 1);
    stat_=STAT_VALID;
}

/**
 * @brief               链接断开时被
 **/
void RedisAccess::on_disconnect()
{
    MON_ADD(ATTR_REDIS_ON_DISCONNECTED, 1);
    stat_=STAT_INVALID;
    /*断开重连*/
    reconnect();
    dc::common::comm_event::CommFactoryInterface::instance().add_comm_redis(this);
}


/**
 * @brief               出错的时候被调用
 **/
void RedisAccess::on_error(const char * errstr)
{
    MON_ADD(ATTR_REDIS_ON_ERROR, 1);
    stat_=STAT_INVALID;
    /*断开重连*/
    reconnect();
    dc::common::comm_event::CommFactoryInterface::instance().add_comm_redis(this);
    return;
}


int RedisPool::init(const std::string & host, int port)
{
    for(int i=0; i< RA_MAX; i++)
    {
        ras_[i].connect(host, port);
        dc::common::comm_event::CommFactoryInterface::instance().add_comm_redis(&(ras_[i]));
    }
    return 0;
}


/**
 * @brief           获取一个RedisAccess实例
 **/
RedisAccess * RedisPool::get_redis_instance(int type)
{
    if(type >= RA_MAX || ras_[type].status() == RedisAccess::STAT_INVALID)
    {
        return NULL;
    }else
    {
        return &(ras_[type]);
    }
}

}//dn
}//poseidon
