#pragma once

#include "boost/serialization/singleton.hpp"
#include <boost/algorithm/string.hpp> 
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/atomic.hpp>
#include <boost/bind.hpp>
#include <hiredis.h>
#include <adapters/libevent.h>
#include "util/timeout_event.h"
#include "util/log.h"
#include "util/timer.h"
#include "util/closure.h"
#include "util/func.h"

using namespace std;

namespace poseidon
{  
namespace util
{

typedef boost::function<void(redisReply *)> FuncOnRedisReply;
void redis_command_cb(redisAsyncContext *context, void *r, void *privdata);

class RedisOnReply
{
  public:
    virtual ~RedisOnReply();
    static RedisOnReply * new_instance(FuncOnRedisReply func);
    void run(redisReply * reply);
  protected:
    FuncOnRedisReply _func_on_redis_reply;
  protected:
    RedisOnReply();
};

struct RedisConnInfo;
class RedisConnChangeInfo;

class RedisClient
{
  public:
    RedisClient();
    virtual ~RedisClient();
    void init(struct event_base * base,string host,int port,bool is_cluster);
    redisAsyncContext * get_redis_context(const string &key);
    void on_connect(int redis_conn_no,int status);
    void on_disconnect(int redis_conn_no,int status);
    void set_reconnect_interval(int reconnect_interval)
    {
      _reconnect_interval=reconnect_interval;
    }
    void set_cluster_info_interval(int cluster_info_interval)
    {
      _cluster_info_interval=cluster_info_interval;
    }
    
  protected:
    int _client_info_num;
    boost::unordered_map<int,boost::shared_ptr<RedisConnInfo> > _redis_conn_infos;
    boost::unordered_map<int,int> _redis_slot_map;
    struct event_base * _base;
    bool _is_init;
    bool _is_cluster;
    bool _init_cluster;
    string _cluster_info_string;
    util::TimeoutEvent * _cluster_info_event;
    int _reconnect_interval;
    int _cluster_info_interval;
    
  protected:
    void connect(boost::shared_ptr<RedisConnInfo> rci);
    void re_connect(boost::shared_ptr<RedisConnInfo> rci);
    void on_connect_cluster(redisReply *reply );
    void get_cluster_info(boost::shared_ptr<RedisConnInfo> rci);
    void get_cluster_info();
    void set_cluster_info_event();
};

}
}