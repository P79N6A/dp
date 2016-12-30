#pragma once

#include "qp_inc.h"

namespace poseidon {
namespace qp {

typedef boost::function<void(redisReply *)> FuncOnRedisReply;
void redis_command_cb(redisAsyncContext *context, void *r, void *privdata);

class RedisOnReply {
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

typedef boost::unordered_map<int, boost::shared_ptr<RedisConnInfo> >::iterator RedisConnIter;
class RedisClient {
public:
    RedisClient();
    virtual ~RedisClient();
    void init(struct event_base * base, string host, int port, bool is_cluster);
    redisAsyncContext * get_redis_context(const string &key);
    void on_connect(int redis_conn_no, int status);
    void on_disconnect(int redis_conn_no, int status);

protected:
    int _client_info_num;
    boost::unordered_map<int, boost::shared_ptr<RedisConnInfo> > _redis_conn_infos;
    boost::unordered_map<int, int> _redis_slot_map;
    struct event_base * _base;
    bool _is_init;
    bool _is_cluster;
    bool _init_cluster;
    string _cluster_info_string;
    util::TimeoutEvent * _cluster_info_event;

protected:
    void connect(boost::shared_ptr<RedisConnInfo> rci);
    void re_connect(boost::shared_ptr<RedisConnInfo> rci);
    void on_connect_cluster(redisReply *reply);
    void get_cluster_info(boost::shared_ptr<RedisConnInfo> rci);
    void get_cluster_info();
    void set_cluster_info_event();
};

}
}
