/**
 */

//include self head file
#include"comm_event.h"

//include STD C head files
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#include"../game_recom/mylog.h"

//include STD C++ head files


//include third_party_lib head files


//include project other head files

namespace dc 
{

namespace common
{
namespace comm_event
{

void CommBase::handle_callback(evutil_socket_t listener, short event, void * arg)
{
    CommBase * pCommBase=(CommBase *)arg;
    if(event & EV_READ)
    {
        char buf[MAX_BUF];
        struct sockaddr_in sin;
        socklen_t slen=sizeof(struct sockaddr_in);
        int n=recvfrom(listener, buf, MAX_BUF, 0, (struct sockaddr *)&sin, &slen);
        if(n<0)
        {
            printf("recvfrom error[%s]\n", strerror(errno));
            return;
        }
        pCommBase->handle_read(buf, n, sin);
    }
}

int CommBase::add_to_base(struct event_base * base)
{
    int rt=EC_SUCCESS;
    do{
        if(base == NULL)
        {
            rt=EC_PARAM_ERROR;
            break;
        }
        if(!isbind_)
        {
            rt=EC_UNBIND_ERROR;
            break;
        }

        listen_event_=event_new(base, listen_sock_, EV_READ|EV_PERSIST, handle_callback, (void *)this);
        event_add(listen_event_, NULL);

    }while(0);
    return rt;
}

/** 
 * @brief               当收到包时的回调函数
 * @param buf           [IN],收到的buf
 * @param len           [IN],buf的长度
 * @param client_addr   [IN],客户端地址
 * @return              成功返回EC_SUCCESS, 否则返回对应错误码
 **/
int CommBase::handle_read(const char * buf, const int len, struct sockaddr_in & client_addr)
{
    return 0;
}


/** 
 * @brief               绑定一个地址
 * @param ip            [IN], ip
 * @param port          [IN], port
 * @return              成功返回EC_SUCCESS, 否则返回对应错误码
 **/
int CommBase::bindaddr(const std::string ip, const uint16_t port )
{
    int rt=EC_SUCCESS;
    do{
        listen_sock_=socket(AF_INET, SOCK_DGRAM, 0);
        if(listen_sock_ < 0)
        {
            rt=EC_SOCKET_ERROR;
            break;
        }

        evutil_make_listen_socket_reuseable(listen_sock_);
        struct sockaddr_in sin;

        sin.sin_family=AF_INET;
        sin.sin_addr.s_addr=inet_addr(ip.c_str());
        sin.sin_port=htons(port);

        if(bind(listen_sock_, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        {
            rt=EC_BIND_ERROR;
            break;
        }
        ip_=ip;
        port_=port;
        isbind_=true;
    }while(0);
    return rt;
}

int CommBase::bindaddr_nb(const std::string ip, const uint16_t port )
{
    int rt=EC_SUCCESS;
    do{
        listen_sock_=socket(AF_INET, SOCK_DGRAM, 0);
        if(listen_sock_ < 0)
        {
            rt=EC_SOCKET_ERROR;
            break;
        }

        evutil_make_listen_socket_reuseable(listen_sock_);
        evutil_make_socket_nonblocking(listen_sock_);
        struct sockaddr_in sin;

        sin.sin_family=AF_INET;
        sin.sin_addr.s_addr=inet_addr(ip.c_str());
        sin.sin_port=htons(port);

        if(bind(listen_sock_, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        {
            rt=EC_BIND_ERROR;
            break;
        }
        ip_=ip;
        port_=port;
        isbind_=true;
    }while(0);
    return rt;
}

/** 
 * @brief               往指定ip发送一个包
 * @param buf           [IN]
 * @param len           [IN]
 * @param client_addr   [IN],客户端地址
 * @return              成功返回EC_SUCCESS, 否则返回对应的错误码
 **/
int CommBase::send_pkg(const char * buf, const int len, struct sockaddr_in & client_addr )
{
    int rt=EC_SUCCESS;
    do{
        if(!isbind_)
        {
            rt=EC_UNBIND_ERROR;
            break;
        }
        int n=sendto(listen_sock_, buf, len, 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr_in));
        if(n<0)
        {
            fprintf(stderr, "sendto error[%s]\n", strerror(errno));
            rt=EC_SEND_ERROR;
            break;
        }
    }while(0);
    return rt;
}


}//comm_event
}//common
}//dc
