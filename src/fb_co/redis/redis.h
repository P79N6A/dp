/*
 * redis.h (v1.0.0)
 * Created on: 2016-12-6
 */

#ifndef CO2_REDIS_REDIS_H_
#define CO2_REDIS_REDIS_H_

#include "hiredis.h"

#include <list>
#include <set>
#include <string>
#include <memory>

#include "co2/sync/cond.h"

namespace co2 {
namespace redis {

class RedisPool;

/* Too hard to write a wrapper of hiredis.. */
class Redis {
public:
    friend class RedisPool;

    Redis();
    ~Redis();

    /* connect to redis server, return 0 if success, non-zero otherwise */
    int Connect(const std::string &host, int port, int db = 0,
        const std::string &passwd = "");

    int Close(void);

    /* send command to the redis-server and wait for the reply */
    redisReply *RedisCommand(const char * fmt, ...);
    redisReply *RedisCommand(const char * fmt, va_list va);

    /* pipeline support, call this function to exit the pipeline mode
     * and return the results */
    int AppendCommand(const char *fmt, ...);
    int AppendCommand(const char *fmt, va_list va);

    std::list<redisReply *> Execute();

private:
    int cmds_;
    int request_;
    redisContext *ctx_;
};

struct RedisConn
{
    std::string host;
    int port;
    int db;
    std::string passwd;
};

class RedisPool {
public:
    RedisPool(int max_connection = 20);
    RedisPool(const std::string &host, int port, int db = 0,
        const std::string &passwd = "", int max_connection = 20);

    RedisPool(const std::list<RedisConn> &servers, int max_connection = 20);
    ~RedisPool();

    int Init(void);

    void AddServer(const std::list<RedisConn> &servers);
    void AddServer(const std::string &hosts, int db = 0,
        const std::string &passwd = "");
    void AddServer(const std::string &host, int port, int db = 0,
        const std::string &passwd = "");

    /* Get Connection from pool, if success a Redis ptr will be return,
     * if the pool has no connection object, the current coroutine will
     * yield until something putback a resource. If error or the pool is
     * closed, NULL will be returned. */
    Redis* GetConnection(void);

    /* Put the redis object back to the pool.
     * TODO: need to check the pointer is legitimated? */
    void Release(Redis *);

    /* close the pool free all the connection objects, if some coroutine
     * has not return the resouce, the current coroutine will be suspended,
     * so never forget to Release the resource before close. */
    void Close(void);

    /* reset the pool to the beginning status, may have to wait */
    void Reset(void);

    /* check if the pool is closed. can not use a closed pool,
     * if want to reuse, call Reset before using it. */
    bool IsClosed(void) { return closed_; }

    void SetMaxConnection(int max);

    int GetMaxConnection(void) { return max_connection_; }

    void SetMaxRequest(int max);

    int GetMaxRequest(void) { return max_request_; }

private:
    int max_connection_;
    int cur_connection_;  /* current connection the pool has */
    int max_request_;     /* a single connection will close after
                            max_requests are performed. */
    std::list<Redis *> avail_;
    std::set<Redis *> using_; /* for validation */
    co2::sync::Cond cond_;
    bool closed_;
    std::list<RedisConn> servers_;
};

}  // namespace redis
}  // namespace co2



#endif /* CO2_REDIS_REDIS_H_ */
