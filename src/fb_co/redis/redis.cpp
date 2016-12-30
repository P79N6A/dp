/*
 * redis.h
 * Created on: 2016-12-6
 */

#include "redis.h"

#include <stdarg.h>
#include <memory.h>
#include <fcntl.h>

#include <boost/algorithm/string.hpp>

#include "util/log.h"
#include "co2/co2.h"

namespace co2 {
namespace redis {

using namespace co2;

Redis::Redis() :
    cmds_(0), request_(0), ctx_(NULL)
{

}

Redis::~Redis()
{
    if (ctx_) {
        redisFree(ctx_);
    }
}

redisReply *Redis::RedisCommand(const char * fmt, ...)
{
    va_list va;
    va_start(va, fmt);

    char *cmd;
    int len = redisvFormatCommand(&cmd, fmt, va);
    va_end(va);

    if (len != 0)
        return NULL; // memory error?

    if (redisAppendFormattedCommand(ctx_, cmd, len) != REDIS_OK) {
        free(cmd);
        return NULL; // format error?
    }
    free(cmd);

    redisReply *reply = NULL;
    redisGetReply(ctx_, (void **)&reply);
    return reply;
}

redisReply *Redis::RedisCommand(const char * fmt, va_list va)
{
    char *cmd;
    int len = redisvFormatCommand(&cmd, fmt, va);
    if (len == -1)
        return NULL;

    if (redisAppendFormattedCommand(ctx_, cmd, len) != REDIS_OK) {
        free(cmd);
        return NULL;
    }
    free(cmd);

    redisReply *reply = NULL;
    redisGetReply(ctx_, (void **)&reply);

    request_++;
    return reply;
}

int Redis::AppendCommand(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char *cmd = NULL;

    int len = redisvFormatCommand(&cmd, fmt, ap);
    va_end(ap);

    int res = redisAppendFormattedCommand(ctx_, cmd, len);
    free(cmd);

    if (res != REDIS_OK)
        return -1;

    cmds_++;
    return 0;
}

int Redis::AppendCommand(const char *fmt, va_list va)
{
    char *cmd = NULL;
    int len = redisvFormatCommand(&cmd, fmt, va);
    int res = redisAppendFormattedCommand(ctx_, cmd, len);
    free(cmd);

    if (res != REDIS_OK)
        return -1;
    cmds_++;
    return 0;
}

int Redis::Connect(const std::string &host, int port, int db,
    const std::string &passwd)
{
    if (!ctx_) {
        ctx_ = redisConnect(host.c_str(), port);
        if (!ctx_) {
            return -1;
        }
        fcntl(ctx_->fd, F_SETFL, fcntl(ctx_->fd, F_GETFL, 0) | O_NONBLOCK);
    }

    /* already connected. */
    redisReply *reply = NULL;
    if (db != 0) {
        reply = RedisCommand("SELECT %d", db);
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            freeReplyObject(reply);
            redisFree(ctx_);
            return kUnknownErr;
        }
        freeReplyObject(reply);
    }
    if (!passwd.empty()) {
        reply = RedisCommand("AUTH %s", passwd.c_str());
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            freeReplyObject(reply);
            redisFree(ctx_);
            return kUnknownErr;
        }
        freeReplyObject(reply);
    }
    return kOK;
}

int Redis::Close(void)
{
    if (ctx_) {
        redisFree(ctx_);
        ctx_ = NULL;
    }
    return 0;
}

std::list<redisReply *> Redis::Execute(void)
{
    std::list<redisReply *> list;
    int i;
    for (i = 0; i < cmds_; ++i) {
        void *reply = NULL;
        redisGetReply(ctx_, (void **) &reply);
        list.push_back((redisReply *) reply);
    }
    cmds_ = 0;
    request_++;
    return list;
}

RedisPool::RedisPool(int max_conn) :
    max_connection_(max_conn), cur_connection_(0), closed_(false)
{

}

RedisPool::RedisPool(const std::string &host, int port, int db,
    const std::string &passwd, int max_conn) :
    max_connection_(max_conn), cur_connection_(0)
{
    closed_ = false;
    AddServer(host, port, db, passwd);
}

RedisPool::RedisPool(const std::list<RedisConn> &servers,
    int max_connection) :
    max_connection_(max_connection), cur_connection_(0), closed_(false)
{
    servers_ = servers;
}

int RedisPool::Init(void)
{
    return cond_.Init();
}

RedisPool::~RedisPool(void)
{
    Close();
}

void RedisPool::SetMaxConnection(int m)
{
    if (m <= cur_connection_)
        m = cur_connection_;
    else
        max_connection_ = m;
}

void RedisPool::SetMaxRequest(int m)
{
    if (m <= 100000) {
        m = 100000;
    }
    max_request_ = m;
}

void RedisPool::Close(void)
{
    while (avail_.size() < (size_t)cur_connection_) {
        cond_.Wait();
    }

    for (auto it = avail_.begin(); it != avail_.end(); ++it) {
        (*it)->Close();
    }

    avail_.clear();
    cur_connection_ = 0;
    closed_ = true;
}

void RedisPool::Reset(void)
{
    Close();
    closed_ = false;
}

void RedisPool::AddServer(const std::string &host, int port, int db,
    const std::string &passwd)
{
    RedisConn conn;
    conn.host = host;
    conn.port = port;
    conn.db = db;
    conn.passwd = passwd;
    servers_.push_back(conn);
}

void RedisPool::AddServer(const std::list<RedisConn> &servers)
{
    for (auto it = servers.begin(); it != servers.end(); ++it) {
        servers_.push_back(*it);
    }
}

void RedisPool::AddServer(const std::string &list,
    int db, const std::string &passwd)
{
    std::vector<std::string> hosts;
    boost::split(hosts, list, boost::is_any_of(","));

    for (size_t i = 0; i < hosts.size(); ++i) {
        std::vector<std::string> host_port;
        boost::split(host_port, hosts[i], boost::is_any_of(":"));

        RedisConn conn;
        conn.host = host_port[0];
        conn.port = atoi(host_port[1].c_str());
        conn.db = db;
        conn.passwd = passwd;

        LOG_INFO("push address %s:%d", conn.host.c_str(), conn.port);
        servers_.push_back(conn);
    }
}

Redis * RedisPool::GetConnection(void)
{
    if (!servers_.size()) {
        /* can not get server without connection */
        return NULL;
    }

    LOG_INFO("servers count = %d", servers_.size());

    Redis *redis = NULL;
    do {
        if (IsClosed())
            return NULL;

        if (!avail_.empty()) {
            /* get connection from pool */
            redis = avail_.front();
            avail_.pop_front();

            /* for validation */
            using_.insert(redis);
            return redis;
        } else if (cur_connection_ < max_connection_) {

            /* create a new one */
            redis = new Redis;
            if (!redis) {
                return NULL;
            }

            /* coroutine will yield here, so let cur_connection increase */
            cur_connection_++;

            /* round robin the server address */
            RedisConn conn = servers_.front();

            LOG_DEBUG("get server %s:%d", conn.host.c_str(), conn.port);

            servers_.pop_front();
            servers_.push_back(conn);

            int res = redis->Connect(conn.host, conn.port, conn.db, conn.passwd);
            if (res != 0) {
                cur_connection_--;
                delete redis;
                return NULL;
            }

            using_.insert(redis);
            return redis;
        } else {
            /* we have to wait now ... */
            LOG_INFO("waiting for redis connection release.");
            cond_.Wait();
        }
    } while (true);
}

void RedisPool::Release(Redis * redis)
{
    auto it = using_.find(redis);
    if (it == using_.end()) {
        /* bad pointer? */
        return;
    }

    LOG_INFO("release redis connection");

    using_.erase(it);
    /* force to reopen connection if requests_ too many times.
     * if we access redis 1000times per second, every 15mins
     * we'll reconnect to redis. */
    if (redis->ctx_->err || redis->request_ > 1000000) {
        delete redis;
        cur_connection_--;
    } else {
        avail_.push_back(redis);
    }
    cond_.Signal();
}

}  // namespace redis
}  // namespace co2

