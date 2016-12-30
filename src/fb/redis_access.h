/**
 **/
#ifndef  _REDIS_ACCESS_H_ 
#define  _REDIS_ACCESS_H_
#include "comm_event_redis.h"
#include <list> 
#include <iostream> 
#include <boost/serialization/singleton.hpp>

namespace poseidon
{
namespace feedback
{
class RedisPool;
typedef int handle_func(void * sess_data, void * pRedisReply, int err, const char * errstr);

class RedisAccess:public dc::common::comm_event::CommRedis
{
public:
    enum
    {
        STAT_VALID=1,       //可用
        STAT_INVALID=2,     //不可用
    };
    
    RedisAccess():stat_(STAT_INVALID)
    {
    }


    void push_handle(handle_func * func)
    {
        func_list_.push_back(func);
    }

    virtual int handle_get_result(void * sess_data, void * pRedisReply);

#if 0
    void set_value(const std::string & key, const std::string & value)
    {
        kv_[key]=value;
    }

    void get_value(const std::string & key, std::string & value)
    {
        value=kv_[key];
    }
#endif
    

    /**
     * @brief               链接完成时被调用
     **/
    virtual void on_connected();

    /**
     * @brief               链接断开时被
     **/
    virtual void on_disconnect();


    /**
     * @brief               出错的时候被调用
     **/
    virtual void on_error(const char * errstr);

    int status()
    {
        return stat_;
    }

private:
    int stat_;

#if 0
    std::map<std::string, std::string> kv_;
#endif

    std::list<handle_func *> func_list_;
};

enum
{
	REDIS_MEMCAMPAIGN=1,
	REDIS_ADKEY=2,
	REDIS_ADVERTISTER=3,
	REDIS_ADNUM=4,
	REDIS_CREATIVE=5,
	REDIS_DEAL_ID=6,
	REDIS_DEAL_CAMPAIGN=7,
	YESTERDAY_COST=8,
	RA_MAX,
};

class RedisPool:public boost::serialization::singleton<RedisPool>
{
public:

    int init(const std::string & host, int port);


    void set_handle(int type, handle_func * handle)
    {
        if(type < RA_MAX && type >=0)
        {
            ras_[type].push_handle(handle);
        }
    }

    /**
     * @brief           获取一个RedisAccess实例
     **/
    RedisAccess * get_redis_instance(int type);

private:
    int ra_cnt_;
    std::list<RedisAccess *> ra_valid_;      //可用的Redis Access
    RedisAccess ras_[RA_MAX];
};



}//qp
}//poseidon

#endif   // ----- #ifndef _REDIS_ACCESS_H_  ----- 


