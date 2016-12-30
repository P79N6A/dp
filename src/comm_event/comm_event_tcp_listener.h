/**
 * @company             
 */

#ifndef  COMM_EVENT_COMM_EVENT_TCP_LISTENER_H_
#define  COMM_EVENT_COMM_EVENT_TCP_LISTENER_H_

#include <iostream>

extern "C"
{
#include "event2/event.h"
#include "event2/bufferevent.h"
#include "event2/listener.h"
}

#include "comm_event_interface.h"
#include "comm_event_tcp.h"


namespace dc
{
namespace common
{
namespace comm_event
{

class CommTcpListener:public CommTcpBase
{
public:

    int listen_on_addr(const std::string ip, const uint16_t port, int backlog);

    virtual int on_accept(evutil_socket_t fd, struct sockaddr *address, int socklen);

    virtual int on_accept_error(); 

    static void accept_conn_cb(struct evconnlistener *listener,
        evutil_socket_t fd, struct sockaddr *address, int socklen,
        void *ctx);

    static void accept_error_cb(struct evconnlistener *listener, void *ctx);

private:
    struct evconnlistener *listener_;
};

}//comm_event
}//common
}//dc

#endif   /* ----- #ifndef COMM_EVENT_COMM_EVENT_TCP_LISTENER_H_  ----- */

