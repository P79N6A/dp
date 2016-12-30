/**
 * @company             
 */

#include "comm_event_tcp_connector.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>



namespace dc
{
namespace common
{
namespace comm_event
{

/** 
 * @brief               绑定本地端口
 * @return              成功返回0，失败返回-1
 **/
int CommTcpConnector::bindaddr(const std::string ip, const uint16_t port )
{
	int rt=EC_SUCCESS;
	int sock=-1;
	do{
		if(status_ != STAT_INITED)
        {
        	rt=EC_STAT_ERR;
        	break;
        }

        sock=socket(AF_INET, SOCK_STREAM, 0);
        if(sock < 0)
        {
            rt=EC_SOCKET_ERROR;
            break;
        }

        evutil_make_listen_socket_reuseable(sock);
        struct sockaddr_in sin;
        sin.sin_family=AF_INET;
        sin.sin_addr.s_addr=inet_addr(ip.c_str());
        sin.sin_port=htons(port);
        if(bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        {
            rt=EC_BIND_ERROR;
            break;
        }
        bufferevent_setfd(bev_, sock);
        if(sock_ >= 0)
        {
        	close(sock_);
        }
        sock_=sock;
	}while(0);
	if(rt != EC_SUCCESS)
    {
    	if(sock > 0)
        {
        	close(sock);
        }
   	}
	return rt;
}

/** 
 * @brief               connect一个远程地址
 **/
int CommTcpConnector::connect(struct sockaddr_in & dst_addr)
{
	int rt=EC_SUCCESS;
	do{
		if(status_ != STAT_INITED && status_ != STAT_CLOSE)
        {
        	rt=EC_STAT_ERR;
        	break;
        }
        if(bufferevent_socket_connect(bev_, (struct sockaddr *)&dst_addr, sizeof(struct sockaddr_in)) < 0)
        {
            rt=EC_CONNECT_ERR;
            break;
        }
        status_=STAT_CONNECTING;
	}while(0);
	return rt;
}

}
}
}
