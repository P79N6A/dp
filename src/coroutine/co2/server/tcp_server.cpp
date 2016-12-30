/*
 * tcpserver.cpp
 * Created on: 2016-11-21
 */

#include "co2/server/tcp_server.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#include <algorithm>

#include "co2/co2.h"
#include "co2/stream/stream.h"

#define BATCH_SIZE 100

namespace co2 {
namespace server {

using co2::stream::Stream;

static void* server_thread(void *arg);
static void* accept_callback(void *arg);

TCPServer::TCPServer(int stack_size, int buf_size, int max_buf_size) :
    accept_fd_(-1), stack_size_(stack_size),
    buf_size_(buf_size), max_buf_size_(max_buf_size),
    server_thread_(NULL), stop_event_(NULL), pool_(NULL),
    running_(true), unix_socket_(false)
{
    int size = 4096;
    while (size < buf_size_ - 1) {
        size <<= 1;
    }

    buf_size_ = size;
    if (max_buf_size_ < size) {
        max_buf_size_ = size;
    }
}

TCPServer::~TCPServer(void)
{
    Stop();

    if (stop_event_) {
        st_cond_destroy(stop_event_);
    }

    if (server_thread_) {
        void *ret = NULL;
        st_thread_join(server_thread_, (void **)&ret);
    }

    if (pool_) {
        /* TODO: how to destroy the pool elegantly? if some coroutine
         * is still using it. */
        pool_->Destroy();
    }
}

Stream * TCPServer::CreateStream(int fd)
{
    Stream *stream = new (std::nothrow)Stream(pool_, fd, max_buf_size_);
    if (!stream)
        return NULL;

    if (0 != stream->Init()) {
        delete stream;
        return NULL;
    }
    return stream;
}

int TCPServer::Start(void)
{
    if (accept_fd_ == -1)
        return kSocketErr;

    if (!pool_) {
        pool_ = new co2::buffer::Pool(buf_size_, BATCH_SIZE);

        if (!pool_)
            return kMemoryErr;
    }

    if (!stop_event_) {
        stop_event_ = st_cond_new();
        if (!stop_event_)
            return kMemoryErr;
    }

    if (!server_thread_) {
        server_thread_ = st_thread_create(server_thread,
            (void *)this, 1, 32768);

        if (!server_thread) {
            return kMemoryErr;
        }
    }
    return kOK;
}

void* server_thread(void *arg)
{
    TCPServer *server = (TCPServer *)arg;

    char host[64];
    int fd;
    int port;
    socklen_t len;

    union {
        struct sockaddr_un un;
        struct sockaddr_in in;
    } sa;

    while (!server->IsStop()) {
        len = server->IsUnixSocket() ?
            sizeof(struct sockaddr_un) : sizeof(struct sockaddr_in);

        fd = accept(server->GetSocket(), (struct sockaddr *)&sa, &len);

        if (fd < 0) {
            if (errno == EINTR)
                continue;
            /* may be it's time to quit. */
            break;
        }

        /* reuse address */
        int reuse = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
        server->GetClients().push_back(fd);

        st_thread_create(accept_callback, (void *)server,
            0, server->GetStackSize());
    }

    return (void *)0;
}

void *accept_callback(void *arg)
{
    TCPServer *server = (TCPServer *)arg;
    if (server->GetClients().empty()) {
        co2::Log(WARN, "no fd in the list\n");
        return (void *)0;
    }

    int fd = server->GetClients().front();
    server->GetClients().pop_front();

    co2::stream::Stream *stream = server->CreateStream(fd);

    if (!stream) {
        close(fd);
        return (void *)0;
    }

    if (kOK != stream->Init()) {
        close(fd);
        delete stream;
        return (void *)0;
    }

    int res = server->OnAccept(*stream);
    return reinterpret_cast<void *>(res);
}

int TCPServer::Stop(void)
{
    void *ret = NULL;
    running_ = false;

    if (accept_fd_ != -1) {
        close(accept_fd_);
        accept_fd_ = -1;
    }

    std::for_each(clients_.begin(), clients_.end(), close);
    clients_.clear();

    if (stop_event_) {
        st_cond_broadcast(stop_event_);
        if (server_thread_) {
            st_thread_join(server_thread_, (void **)&ret);
            server_thread_ = NULL;
        }
    }

    co2::Log(INFO, "server stopped\n");
    return ret == NULL ? kOK : kUnknownErr;
}

int TCPServer::Bind(const std::string & addr, int port, int backlog)
{
    if (accept_fd_ != -1)
        return kOK;

    accept_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (accept_fd_ == -1)
        return kSocketErr;

    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = inet_addr(addr.c_str());

    /* reused address */
    int reuse = 1;
    setsockopt(accept_fd_, SOL_SOCKET, SO_REUSEADDR,
        (const char *)&reuse, sizeof(int));

    int res;
    socklen_t len = sizeof(struct sockaddr_in);

    int i = 0;
    while (++i < 10) {
        /* sometimes even reused address is set, bind could fail,
         * so try 10 times before we return bind error. */
        res = bind(accept_fd_, (struct sockaddr *)&sa, len);
        if (res != 0) {
            sleep(1);
            continue;
        }
        break;
    }
    if (i >= 10)
        return kBindErr;

    res = listen(accept_fd_, backlog);
    if (res != 0)
        return kListenErr;

    return kOK;
}

int TCPServer::BindUnix(const std::string &path, int backlog)
{
    if (accept_fd_ != -1)
        return kOK;

    int res;
    res = unlink(path.c_str());
    if (res != 0 && errno != ENOENT) {
        return kUnlinkErr;
    }

    accept_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (!accept_fd_)
        return kSocketErr;

    struct sockaddr_un sa;
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, path.c_str(), path.size());
    sa.sun_path[path.size()] = 0;

    /* reused address */
    int reuse = 1;
    setsockopt(accept_fd_, SOL_SOCKET, SO_REUSEADDR,
        (const char *)&reuse, sizeof(int));

    int i = 0;
    while (++i <= 10) {
        /* sometimes even reused address is set, bind could fail,
         * so try 10 times before we return bind error. */
        res = bind(accept_fd_, (struct sockaddr *)&sa,
            sizeof(struct sockaddr_un));

        if (res != 0) {
            fprintf(stderr, "Bind error: %s\n", strerror(errno));
            sleep(1);
            continue;
        }
        break;
    }
    if (i >= 10)
        return kBindErr;

    res = listen(accept_fd_, backlog);
    if (res != 0)
        return kListenErr;

    unix_socket_ = true;
    return kOK;
}

int TCPServer::ServeForever()
{
    int res = Start();
    if (res != kOK)
        return res;

    void *val = NULL;
    st_cond_wait(stop_event_);
    return 0;
}

int TCPServer::OnAccept(co2::stream::Stream &stream)
{
    co2::Log(INFO, "on accept\n");
    return 0;
}

}  // namespace server
}  // namespace co2

