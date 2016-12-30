/**
 * @company             
 */

#include "comm_event_tcp_listener.h"
#include <arpa/inet.h>

namespace dc
{
namespace common
{
namespace comm_event
{

#define MakeAddr(addr, ip, port) \
do{\
    addr.sin_family = AF_INET;\
    addr.sin_port = htons((short)port);\
    addr.sin_addr.s_addr = inet_addr(ip);\
}while(0)


int CommTcpListener::listen_on_addr(const std::string ip, const uint16_t port, int backlog)
{
	int rt=0;
	do{
        struct sockaddr_in sin;
        MakeAddr(sin, ip.c_str(), port);
        if(base_ == NULL)
        {
        	rt=-1;
        	break;
        }

        listener_ = evconnlistener_new_bind(base_, accept_conn_cb, this,
                LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, backlog,
                (struct sockaddr*)&sin, sizeof(sin));
        if (!listener_)
        {
            perror("Couldn't create listener");
            rt = -2;
        }
        evconnlistener_set_error_cb(listener_, accept_error_cb);
        
	}while(0);
	return rt;
}

int CommTcpListener::on_accept(evutil_socket_t fd, struct sockaddr *address, int socklen)
{
	int rt=0;
	do{

	}while(0);
	return rt;
}

int CommTcpListener::on_accept_error()
{
	int rt=0;
	do{
	}while(0);
	return rt;
}

void CommTcpListener::accept_conn_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *address, int socklen, void *ctx)
{
	CommTcpListener * ins= (CommTcpListener*) ctx;
    ins->on_accept(fd, address, socklen);
}

void CommTcpListener::accept_error_cb(struct evconnlistener *listener, void *ctx)
{
	CommTcpListener * ins = (CommTcpListener*) ctx;
    ins->on_accept_error();
}

}

}
}


