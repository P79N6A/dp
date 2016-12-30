/*
 * hook.c
 * Created on: 2016-12-09
 */

#include <stdio.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <pthread.h>

#include "st.h"
#include "common.h"

#define GLIBC_HOOK(func) \
    if (!glibc_##func) { \
        glibc_##func = (co_##func##_t)dlsym(RTLD_NEXT, #func); \
    }

co_close_t glibc_close = NULL;
co_connect_t glibc_connect = NULL;
co_socket_t glibc_socket = NULL;
co_accept_t glibc_accept = NULL;
co_read_t glibc_read = NULL;
co_recv_t glibc_recv = NULL;
co_write_t glibc_write = NULL;
co_recvfrom_t glibc_recvfrom = NULL;
co_send_t glibc_send = NULL;
co_sendto_t glibc_sendto = NULL;
co_setsockopt_t glibc_setsockopt = NULL;
co_getsockopt_t glibc_getsockopt = NULL;

st_netfd_t state_thread_fds[65535];

/* TODO: add the other function call if needed. */
void hook_library(void)
{
    GLIBC_HOOK(socket);
    GLIBC_HOOK(close);
    GLIBC_HOOK(connect);
    GLIBC_HOOK(accept);
    GLIBC_HOOK(read);
    GLIBC_HOOK(recv);
    GLIBC_HOOK(recvfrom);
    GLIBC_HOOK(write);
    GLIBC_HOOK(send);
    GLIBC_HOOK(sendto);
    GLIBC_HOOK(setsockopt);
    GLIBC_HOOK(getsockopt);
}

int socket(int domain, int type, int protocol)
{
    GLIBC_HOOK(socket);

    st_log("st_socket: domain = %d, type = %d\n", domain, type);

    int s = glibc_socket(domain, type, protocol);
    if (s < 0) {
        return -1;
    }

    /* dont' hook the socket if not in the same thread. */
    if (syscall(SYS_gettid) != getpid())
        return s;

    st_netfd_t nfd = st_netfd_open_socket(s);
    if (nfd) {
        state_thread_fds[s] = nfd;
        return s;
    }
    return -1;
}

int close(int fd)
{
    GLIBC_HOOK(close);

    st_log("st_close: %d\n", fd);

    st_netfd_t nfd = state_thread_fds[fd];
    int res;
    if (nfd) {
        res = st_netfd_close(nfd);
        state_thread_fds[fd] = NULL;
    } else {
        res = glibc_close(fd);
    }
    return res;
}

int connect(int fd, const struct sockaddr *sa, socklen_t len)
{
    GLIBC_HOOK(connect);

    st_log("st_connect: fd = %d\n", fd);

    st_netfd_t nfd = state_thread_fds[fd];
    if (!nfd) {
        return glibc_connect(fd, sa, len);
    }
    return st_connect(nfd, sa, len, nfd->write_timeout);
}

int accept(int fd, struct sockaddr *sa, socklen_t *len)
{
    GLIBC_HOOK(accept);

    st_log("st_accept: fd = %d\n", fd);

    /* server net fd */
    st_netfd_t sfd = state_thread_fds[fd];
    if (!sfd) {
        return glibc_accept(fd, sa, len);
    }

    /* client net fd */
    st_netfd_t cfd = st_accept(sfd, sa, (int *)len, sfd->read_timeout);
    if (!cfd) {
        return -1;
    }
    state_thread_fds[cfd->osfd] = cfd;
    return cfd->osfd;
}

ssize_t read(int fd, void *buf, size_t len)
{
    GLIBC_HOOK(read);

    st_log("st_read: fd = %d\n", fd);

    st_netfd_t nfd = state_thread_fds[fd];
    if (!nfd) {
        st_log("no netfd found: %d\n", fd);
        return glibc_read(fd, buf, len);
    }
    return st_read(nfd, buf, len, nfd->read_timeout);
}

/* TODO: no recv in state-thread, add it later on. */
ssize_t recv(int fd, void *buf, size_t len, int flags)
{
    GLIBC_HOOK(recv);

    st_log("st_recv: fd = %d\n", fd);

    st_netfd_t nfd = state_thread_fds[fd];
    if (!nfd) {
        return glibc_recv(fd, buf, len, flags);
    }
    return st_read(nfd, buf, len, nfd->read_timeout);
}

ssize_t recvfrom(int fd, void *buf, size_t nbyte, int flags,
    struct sockaddr *sa, socklen_t *len)
{
    GLIBC_HOOK(recvfrom);

    st_log("st_recvfrom: fd = %d\n", fd);

    st_netfd_t nfd = state_thread_fds[fd];
    if (!nfd) {
        return glibc_recvfrom(fd, buf, nbyte, flags, sa, len);
    }
    return st_recvfrom(nfd, buf, nbyte, sa, (int *)len, nfd->read_timeout);
}

ssize_t write(int fd, const void *buf, size_t nbyte)
{
    GLIBC_HOOK(write);

    st_log("st_write: fd = %d\n", fd);

    st_netfd_t nfd = state_thread_fds[fd];
    if (!nfd) {
        st_log("no netfd found: %d\n", fd);
        return glibc_write(fd, buf, nbyte);
    }
    return st_write(nfd, buf, nbyte, nfd->write_timeout);
}

/* TODO: no send function in state thread, add it later on. */
ssize_t send(int fd, const void *buf, size_t nbyte, int flags)
{
    GLIBC_HOOK(send);

    st_log("st_send: fd = %d\n", fd);

    st_netfd_t nfd = state_thread_fds[fd];
    if (!nfd) {
        return glibc_send(fd, buf, nbyte, flags);
    }
    return st_write(nfd, buf, nbyte, nfd->write_timeout);
}

ssize_t sendto(int fd, const void *buf, size_t nbyte,
    int flags, const struct sockaddr *sa, socklen_t len)
{
    GLIBC_HOOK(sendto);

    st_log("st_sendto: fd = %d\n", fd);

    st_netfd_t nfd = state_thread_fds[fd];
    if (!nfd) {
        return glibc_sendto(fd, buf, nbyte, flags, sa, len);
    }
    return (ssize_t)st_sendto(nfd, buf, nbyte, sa, len, nfd->write_timeout);
}

int getsockopt(int fd, int level, int optname,
    void *optval, socklen_t *optlen)
{
    GLIBC_HOOK(getsockopt);

    st_log("st_getsockopt: fd = %d\n", fd);

    if ((optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) &&
        level == SOL_SOCKET) {

        st_netfd_t nfd = state_thread_fds[fd];
        if (!nfd) {
            errno = EINVAL;
            return -1;
        }

        struct timeval *tv = (struct timeval *)optval;
        if (optname == SO_RCVTIMEO) {
            tv->tv_sec = nfd->read_timeout / 100000;
            tv->tv_usec = nfd->read_timeout % 100000;
        } else {
            tv->tv_sec = nfd->write_timeout / 100000;
            tv->tv_usec = nfd->write_timeout % 100000;
        }
        return 0;
    } else {
        return glibc_getsockopt(fd, level, optname, optval, optlen);
    }
}

int setsockopt(int fd, int level, int optname,
    const void *optval, socklen_t optlen)
{
    GLIBC_HOOK(setsockopt);

    st_log("st_setsockopt: fd = %d\n", fd);

    if ((optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) &&
            level == SOL_SOCKET) {
        st_netfd_t nfd = state_thread_fds[fd];
        if (!nfd) {
            errno = EINVAL;
            return -1;
        }

        struct timeval *tv = (struct timeval *)optval;
        if (optname == SO_RCVTIMEO) {
            if (tv) nfd->read_timeout = tv->tv_sec * 100000 + tv->tv_usec;
            else nfd->read_timeout = -1;
        } else {
            if (tv) nfd->write_timeout = tv->tv_sec * 100000 + tv->tv_usec;
            else nfd->write_timeout = -1;
        }
        return 0;
    } else {
        return glibc_setsockopt(fd, level, optname, optval, optlen);
    }
}

