#pragma once

#include "qp_inc.h"
#include "qp_context.h"
#include "redis_client.h"
namespace poseidon {
namespace qp {

typedef boost::unordered_map<string, boost::shared_ptr<QpContext> >::iterator ContextMapIter;

class QpWorker {
public:
    QpWorker() {
        _base = NULL;
        _serial_num = 0;
    }
    virtual ~QpWorker() {
        if (_base != NULL) {
            event_base_free(_base);
        }
    }
    void start_up(int sock_fd);
    void process(char * recv_buf, int recv_len, const struct sockaddr_in &addr);

protected:
    struct event_base* _base;
    int _sock_fd;
    uint64_t _serial_num;
    boost::unordered_map<string, boost::shared_ptr<QpContext> > _context_map;
    RedisClient _redis_client;
    boost::function<void(boost::shared_ptr<QpContext> context)> _func_process;

protected:
    bool add_context_map(boost::shared_ptr<QpContext>);
    void timeout_process(boost::shared_ptr<QpContext> context);
    void get_user_tag(boost::shared_ptr<QpContext> context);
    void on_got_user_tag(const string &context_id, redisReply *reply);
    void get_user_id(boost::shared_ptr<QpContext> context);
    void on_got_user_id(const string &context_id, redisReply *reply);
    void done(boost::shared_ptr<QpContext> context);
    void get_local_tag(boost::shared_ptr<QpContext> context);
    void get_price_tag(boost::shared_ptr<QpContext> context);
    void get_ip_tag(boost::shared_ptr<QpContext> context);
    void set_timeout(boost::shared_ptr<QpContext> context);
};

}
}
