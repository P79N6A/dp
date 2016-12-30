/*
 * udp_server.cpp
 * Created on: 2016-11-24
 */

#include "co2/server/udp_server.h"

#include <unistd.h>
#include <memory.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <st/st.h>
#include <errno.h>
#include <string.h>

#include <string>

#include "co2/co2.h"

namespace co2 {
namespace server {

using namespace co2;

static void *datagram_callback(void *);
static void *server_thread(void *);

UDPServer::UDPServer(int stack_size, int packet_size) :
    accept_fd_(-1), stack_size_(stack_size), pool_(NULL),
    packet_size_(packet_size), port_(-1), running_(true),
    server_thread_(NULL), stop_event_(NULL)
{

}

UDPServer::~UDPServer(void)
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
		pool_->Destroy();
	}
}

int UDPServer::Bind(const std::string &addr, int port)
{
    if (accept_fd_ == -1) {
        accept_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (accept_fd_ == -1) {
            return kSocketErr;
        }
    }

    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = inet_addr(addr.c_str());

    socklen_t len = sizeof(struct sockaddr_in);

    int i = 0;
    while (i < 10) {
        int res = bind(accept_fd_, (struct sockaddr *)&sa, len);
        if (res != 0) {
            co2::Log(WARN, "bind %d: %s\n", errno, strerror(errno));
            i++;
            sleep(1);
            continue;
        }
        break;
    }
    if (i == 10)
        return kBindErr;

    port_ = port;
    return kOK;
}

int UDPServer::Start(void)
{
    if (!pool_) {
        pool_ = new co2::buffer::Pool(packet_size_ +
            sizeof(struct sockaddr_in), 100);

        if (!pool_)
            return kMemoryErr;
    }

    if (!stop_event_) {
        stop_event_ = st_cond_new();
        if (!stop_event_) {
            return kMemoryErr;
        }
    }

    if (!server_thread_) {
        server_thread_ = st_thread_create(server_thread,
            (void *)this, 1, 64 * 1024);

        if (!server_thread_) {
            return kMemoryErr;
        }
    }
    return kOK;
}

int UDPServer::ServeForever()
{
    int res = Start();
    if (res != kOK)
        return res;

    void *val = NULL;
    st_cond_wait(stop_event_);
    return 0;
}

void *server_thread(void *arg)
{
    UDPServer *server = (UDPServer *)arg;

    co2::buffer::Pool *pool = server->GetPool();
    int packet_size = server->GetPacketSize();

    struct sockaddr_in addr;
    socklen_t len = sizeof(struct sockaddr_in);

    while (!server->IsStop()) {
        struct co2::buffer::buffer_ *buf = pool->GetBuffer();
        if (!buf) {
            co2::Log(INFO, "no enough memory!\n");
            break;
        }

        int r = recvfrom(server->GetSocket(),
            ((char *)buf->buf) + sizeof(struct sockaddr_in),
                packet_size, 0, (struct sockaddr*)&addr, &len);

        if (r > 0) {
            /* reuse buffer structure, a bit strange but work. */
            *(struct sockaddr_in *)(buf->buf) = addr;
            buf->size = r;

            if (!st_thread_create(datagram_callback, (void *)server,
                0, server->GetStackSize())) {
                co2::Log(WARN, "create coroutine failed! packet is dropped.");
                pool->Release(buf);
            } else {
                /* when the coroutine is run, it will get the
                 * buffer from server's queue*/
                server->GetClients().push_back(buf);
            }
        } else if (r <= 0) {
            /* error happens, remember to release the buffer. */
            co2::Log(WARN, "read failed[%d], code= %d, err = %s\n",
                r, errno, strerror(errno));

            pool->Release(buf);
            continue;
        }
    }

    /* TODO: more cleanups here? */
    return (void *)0;
}

void *datagram_callback(void *arg)
{
    UDPServer *server = (UDPServer *)arg;

    std::deque<co2::buffer::buffer_ *> &clients = server->GetClients();

    if (clients.empty()) {
        co2::Log(WARN, "no addr in the list\n");
        return (void *)0;
    }

    struct co2::buffer::buffer_ *buf = clients.front();
    clients.pop_front();

    std::string host = inet_ntoa(((struct sockaddr_in *)(buf->buf))->sin_addr);
    int port = htons(((struct sockaddr_in *)(buf->buf))->sin_port);

    co2::Log(INFO, "client [%s:%d] on datagram, datasize = %d\n",
        host.c_str(), port, buf->size);

    server->OnDatagram(host, port, (char *)buf->buf +
        sizeof(struct sockaddr_in), buf->size);

    server->GetPool()->Release(buf);
    return (void *)0;
}

int UDPServer::Stop(void)
{
    void *ret = NULL;
    running_ = false;

    if (accept_fd_ != -1) {
        close(accept_fd_);
        accept_fd_ = -1;
    }

    for (auto it = clients_.begin(); it != clients_.end(); ++it) {
        pool_->Release(*it);
    }

    clients_.clear();

    if (stop_event_) {
        st_cond_broadcast(stop_event_);
        if (server_thread_) {
            st_thread_join(server_thread_, &ret);
            server_thread_ = NULL;
        }
    }

    co2::Log(INFO, "server stopped\n");
    return ret == NULL ? kOK : kUnknownErr;
}

int UDPServer::OnDatagram(const std::string &host, int port,
    const char *buf, int size)
{
    return 0;
}

int UDPServer::WriteBytes(const std::string &host, int port, 
    const void *buf, int size)
{
    struct sockaddr_in ai;
    ai.sin_family = AF_INET;
    ai.sin_port = htons(port);
    ai.sin_addr.s_addr = inet_addr(host.c_str());

    return sendto(accept_fd_, buf, size, 0,
        (struct sockaddr *)&ai, sizeof(struct sockaddr_in));
}

}  // namespace server
}  // namespace co2


