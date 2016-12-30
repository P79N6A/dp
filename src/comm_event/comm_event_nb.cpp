/**
 * zhangxianghui modify
 */

//include self head file
#include"comm_event_nb.h"
#include "event2/event_compat.h"

//include STD C head files
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <fcntl.h>
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

void CommBase_nb::handle_callback(evutil_socket_t listener, short event, void * arg)
{
    CommBase_nb * pCommBase_nb=(CommBase_nb *)arg;
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
        pCommBase_nb->handle_read(buf, n, sin);
    }
    if(event & EV_WRITE) 
    {
    	pCommBase_nb->OnUdpWrite(listener);
    }
}

int CommBase_nb::add_to_base(struct event_base * base)
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
		event_set(&wevt,listen_sock_,EV_WRITE,handle_callback,this);
		event_set(&revt,listen_sock_,EV_READ,handle_callback,this);
	    event_base_set(base,&revt);
	    event_base_set(base,&wevt);		
        event_add(&revt, NULL);//enable read event
        event_add(&wevt, NULL);//enabel write event

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
int CommBase_nb::handle_read(const char * buf, const int len, struct sockaddr_in & client_addr)
{
    return 0;
}


/** 
 * @brief               绑定一个地址
 * @param ip            [IN], ip
 * @param port          [IN], port
 * @return              成功返回EC_SUCCESS, 否则返回对应错误码
 **/
int CommBase_nb::bindaddr(const std::string ip, const uint16_t port )
{
    int rt=EC_SUCCESS;
    do{
        listen_sock_=socket(AF_INET, SOCK_DGRAM, 0);
        if(listen_sock_ < 0)
        {
            rt=EC_SOCKET_ERROR;
            break;
        }
	    int flags = fcntl(listen_sock_, F_GETFL, 0);
	    flags |= O_NONBLOCK;
	    fcntl(listen_sock_, F_SETFL, flags);

        evutil_make_listen_socket_reuseable(listen_sock_);
        //evutil_make_socket_nonblocking(listen_sock_);
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
int CommBase_nb::send_pkg(const char * buf, const int len, struct sockaddr_in & client_addr )
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
	        if(errno == EAGAIN || errno == EWOULDBLOCK)
	        {
	            if(udp_send_queue_.size() > 65535)
	            {
	                fprintf(stderr, "Udp send queue is too long!!\n");
	                break;
	            }	            
	            udp_send_queue_.push(UdpSegment());
	            UdpSegment &back = udp_send_queue_.back();//取出刚push的引用
	            back.buff = (char*)malloc(len);
	            back.buff_len = len;
	            memcpy(back.buff, buf, len);
	            back.dst = client_addr;
	            event_add(&wevt,NULL);//打开写
	        }
	        else {	        	        	
	            fprintf(stderr, "sendto error[%s]\n", strerror(errno));
	            rt=EC_SEND_ERROR;	            
        	}
        	break;
        }
    }while(0);
    return rt;
}


void CommBase_nb::OnUdpWrite(evutil_socket_t listener)
{
    while(udp_send_queue_.size() > 0)
    {
        //从队列第一个开始发送
        UdpSegment &udp_seg = udp_send_queue_.front();
        int ret = sendto(listener, udp_seg.buff, udp_seg.buff_len
                         , 0, (struct sockaddr *)&udp_seg.dst, static_cast<socklen_t>(sizeof(struct sockaddr)));
        if(ret > 0)
        {
        	free(udp_seg.buff);
            udp_send_queue_.pop();//成功发送就继续下一个
        }
        else
        {
            break;
        }
    }
    
    if(udp_send_queue_.size() == 0)
    {
    	event_del(&wevt);//不需要发送堆积的数据了，关闭写事件
    }
}


}//comm_event
}//common
}//dc
