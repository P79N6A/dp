/*
 * stream.cpp
 * Created on: 2016-11-21
 */

#include "co2/stream/stream.h"

#include <unistd.h>
#include <memory.h>
#include <limits.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <st/st.h>

#include <string>
#include <algorithm>

#include "co2/co2.h"


namespace co2 {
namespace stream {

Stream::Stream(Pool *pool, int fd, uint32_t max_buf_size) :
    pool_(pool), fd_(fd),
    rbuf_(pool, max_buf_size), wbuf_(pool, max_buf_size),
    read_timeout_(-1), write_timeout_(-1)
{

}

Stream::~Stream(void)
{
    if (fd_ != -1) {
        close(fd_);
    }
}

int Stream::Init(void)
{
    if (fd_ == -1) {
        return kValueErr;
    }
    return kOK;
}

int Stream::Connect(const std::string &addr, int port, int timeout)
{
    if (fd_ != -1) {
        /* fd == -1, should close before connect */
        return kSocketErr;
    }

    fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd_ == -1) {
        return kSocketErr;
    }

    /* fd is non-block now */
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = ntohs(port);
    sa.sin_addr.s_addr = inet_addr(addr.c_str());

    if (timeout != -1) {
        struct timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = timeout * 1000;
        setsockopt(fd_, SOL_SOCKET, SO_SNDTIMEO, (void *)&tv, sizeof(tv));
    }

    int res = connect(fd_, (struct sockaddr *)&sa, sizeof(sa));
    if (res == -1) {
        /* get errno to see what happens */
        co2::Log(WARN, "connect failed: %s", strerror(errno));

        return errno == ETIME ? kTimeoutErr : kConnectErr;
    } else {
        /* clear the timeout */
        if (timeout == -1) {
            setsockopt(fd_, SOL_SOCKET, SO_SNDTIMEO, NULL, 0);
        }
    }

    /* ok, we are connected. */
    return kOK;
}

int Stream::ConnectUnix(const std::string &path, int timeout)
{
    if (fd_ != -1) {
        /* fd == -1, should close before connect */
        return kSocketErr;
    }

    fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd_ == -1) {
        return kSocketErr;
    }

    /* fd is non-block now */
    struct sockaddr_un sa;
    sa.sun_family = AF_INET;
    memcpy(sa.sun_path, path.c_str(), path.size());
    sa.sun_path[path.size()] = 0;

    if (timeout != -1) {
        struct timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = timeout * 1000;
        setsockopt(fd_, SOL_SOCKET, SO_SNDTIMEO, (void *)&tv, sizeof(tv));
    }

    int res = connect(fd_, (struct sockaddr *)&sa, sizeof(sa));
    if (res == -1) {
        /* get errno to see what happens */
        return kConnectErr;
    } else {
        /* clear the timeout */
        if (timeout == -1) {
            setsockopt(fd_, SOL_SOCKET, SO_SNDTIMEO, NULL, 0);
        }
    }

    /* ok, we are connected. */
    fd_ = res;
    return kOK;
}

int Stream::ReadBytes(int bytes, std::string &buf, int timeout)
{
    // read_type_ = kReadBytes;
    if (fd_ == -1)
        return kSocketErr;

    if (timeout != read_timeout_)
        SetReadTimeout(timeout);

    return rbuf_.ReadBytes(fd_, buf, bytes);
}

int Stream::ReadLine(std::string &buf, int timeout)
{
    if (timeout != read_timeout_)
        SetReadTimeout(timeout);

    return rbuf_.ReadLine(fd_, buf);
}

int Stream::WriteBytes(const char *buf, int bytes, int timeout)
{
    if (write_timeout_ != timeout)
        SetWriteTimeout(timeout);

    int w = write(fd_, buf, bytes);
    if (w == bytes)
        return kOK;

    return kWriteErr;
}

int Stream::WriteBytes(const std::string &buf, int timeout)
{
    return WriteBytes(buf.c_str(), buf.size(), timeout);
}

int Stream::AppendBytes(const char *buf, int bytes)
{
    return wbuf_.WriteBytes(buf, bytes);
}

int Stream::Flush(int timeout)
{
    if (write_timeout_ != timeout)
        SetWriteTimeout(timeout);

    return wbuf_.Flush(fd_);
}

int Stream::SetReadTimeout(int timeout)
{
    read_timeout_ = timeout;
    if (timeout == -1) {
        /* clear timeout indeed */
        return setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, NULL, 0);
    } else {
        struct timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = timeout * 1000;
        return setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO,
            (void *)&tv, sizeof(tv));
    }
}

int Stream::SetWriteTimeout(int timeout)
{
    write_timeout_ = timeout;
    if (timeout == -1) {
        /* clear timeout indeed */
        return setsockopt(fd_, SOL_SOCKET, SO_SNDTIMEO, NULL, 0);
    } else {
        struct timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = timeout * 1000;
        return setsockopt(fd_, SOL_SOCKET, SO_SNDTIMEO,
            (void *)&tv, sizeof(tv));
    }
}

int Stream::Close(void)
{
    fprintf(stderr, "Close\n");
    if (fd_ != -1) {
        close(fd_);
        fd_ = -1;
    }

    /* how to free the task that are running? */
    return kOK;
}

static void GetSockName(int fd, int &port, std::string &host)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int res = getsockname(fd, (struct sockaddr *)&addr, &len);
    if (res == -1) {
        port = -1;
        host = "unknown";
        return;
    }
    port = ntohs(addr.sin_port);
    host = inet_ntoa(addr.sin_addr);
}

static void GetPeerName(int fd, int &port, std::string &host)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int res = getpeername(fd, (struct sockaddr *)&addr, &len);
    if (res == -1) {
        port = -1;
        host = "unknown";
        return;
    }
    port = ntohs(addr.sin_port);
    host = inet_ntoa(addr.sin_addr);
}

const std::string& Stream::GetLocalHost()
{
    if (local_host_.empty()) {
        GetSockName(fd_, local_port_, local_host_);
    }
    return local_host_;
}

const std::string& Stream::GetRemoteHost()
{
    if (remote_host_.empty()) {
        GetPeerName(fd_, remote_port_, remote_host_);
    }
    return remote_host_;
}

int Stream::GetLocalPort()
{
    if (fd_ == -1) {
        return -1;  /* not initialized */
    }

    if (!local_port_) {
        GetSockName(fd_, local_port_, local_host_);
    }
    return local_port_;
}

int Stream::GetRemotePort()
{
    if (fd_ == -1) {
        return -1; /* not initialized */
    }
    if (!remote_port_) {
        GetPeerName(fd_, remote_port_, remote_host_);
    }
    return remote_port_;
}

}  // namespace stream
}  // namespace co2





