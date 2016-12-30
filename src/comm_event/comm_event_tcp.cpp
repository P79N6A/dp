/**
 * @company             
 */

#include "comm_event_tcp.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "trace.h" 

#ifndef LOG_ERR
#define LOG_ERR(fmt, a...) fprintf(stderr, "[%d in %s]"fmt"\n", __LINE__, __FILE__, ##a)
#endif


namespace dc 
{

namespace common
{
namespace comm_event
{

CommTcpBase::~CommTcpBase()
{
    if(bev_ != NULL)
    {
    	bufferevent_free(bev_);
    	bev_=NULL;
    }
    if(sock_ >= 0)
    {
    	sock_=-1;
    }
    base_=NULL;
}

/** 
 * @brief               进行基本的初始化
 * @return              成功返回EC_SUCCESS, 否则返回对应的错误码
 **/
int CommTcpBase::init(int fd, int status)
{
	int rt=EC_SUCCESS;
	do{
		if(base_ ==  NULL)
        {
        	rt=EC_STAT_ERR;
        	break;
        }

        bev_=bufferevent_socket_new(base_, fd, BEV_OPT_CLOSE_ON_FREE);
        if(bev_ == NULL)
        {
            rt=EC_NEW_SOCKET_ERR;
            break;
        }
        bufferevent_setcb(bev_, read_cb, NULL, event_cb, (void *)this);
        status_=status;
        bufferevent_enable(bev_, EV_READ|EV_WRITE);
        sock_=fd;
	}while(0);
	return rt;
}


/** 
 * @brief               连接成功时调用
 **/
int CommTcpBase::on_connected()
{
	int rt=EC_SUCCESS;
	do{
	}while(0);
	return rt;
}


/** 
 * @brief               读到数据时调用
 * @return
 **/
int CommTcpBase::on_read_ready(const char * buf, const int len)
{
	int rt=EC_SUCCESS;
	do{
	}while(0);
	return rt;
}


/** 
 * @brief               判断读到的数据是否是一个完整的包
 * @return              true，包完整，handle_pkg将被调用, false, 包不是完整的包
 **/
bool CommTcpBase::read_done(const char * buf, const int len)
{
	int rt=true;
	do{
	}while(0);
	return rt;
}


/** 
 * @brief               当读到一个完整的包是调用这个函数
 * @return
 **/
int CommTcpBase::handle_pkg(const char * buf, const int len)
{
	int rt=EC_SUCCESS;
	do{
	}while(0);
	return rt;
}


/** 
 * @brief               
 * @param
 * @return
 **/
int CommTcpBase::write(const char * buf, const int len)
{
	int rt=EC_SUCCESS;
	do{
        bufferevent_write(bev_, buf, len);
        bufferevent_enable(bev_, EV_READ);
        read_cnt_=0;
	}while(0);
	return rt;
}

/** 
 * @brief               把当前event加到event_base里面去
 * @param base          [IN]
 * @return              成功返回EC_SUCCESS, 否则返回对应的错误码
 **/
int CommTcpBase::add_to_base(struct event_base * base)
{
	int rt=EC_SUCCESS;
	do{
        if( base_!= NULL || base == NULL)
        {
            rt=EC_PARAM_ERROR;
            break;
        }
        if(status_ != STAT_UNVALID)
        {
        	rt=EC_STAT_ERR;
        	break;
        }
        base_=base;

	}while(0);
	return rt;
}

/** 
 * @brief              发生错误时调用
 * @param
 * @return
 **/
int CommTcpBase::on_error()
{
	TRACE;
	int rt=0;
	do{
	}while(0);
	return rt;
}

/** 
 * @brief              close时调用
 * @param
 * @return
 **/
int CommTcpBase::on_close()
{
	TRACE;
	int rt=0;
	do{
	}while(0);
	return rt;
}

/** 
 * @brief              timeout调用
 * @param
 * @return
 **/
int CommTcpBase::on_timeout()
{
	TRACE;
	int rt=0;
	do{
	}while(0);
	return rt;
}

void CommTcpBase::event_cb(struct bufferevent * bev, short event, void * arg)
{
	TRACE;
    int rt=0;
    CommTcpBase * pBase=(CommTcpBase *)arg;
    do{
        if(event & BEV_EVENT_CONNECTED)
        {
        	if(pBase->status_ != STAT_CONNECTING)
            {
            	LOG_ERR("connect success, but status is not STAT_CONNECTING\n");
            	rt=EC_STAT_ERR;
            	break;
            }
            pBase->status_=STAT_CONNECTED;
            rt=pBase->on_connected();
            if(rt != 0)
            {
                LOG_ERR("on_connected error\n");
                break;
            }
        }else if(event & BEV_EVENT_ERROR)
        {
        	pBase->status_=STAT_CLOSE;
        	rt=pBase->on_error();
        	if(rt != 0)
            {
                LOG_ERR("on_error error\n");
            }
        }else if(event & BEV_EVENT_EOF)
        {
        	pBase->status_=STAT_CLOSE;
        	rt=pBase->on_close();
        	if(rt != 0)
            {
                LOG_ERR("on_close error\n");
            }
        }else if(event & BEV_EVENT_TIMEOUT)
        {
        	pBase->status_=STAT_CLOSE;
        	rt=pBase->on_timeout();
        	if(rt != 0)
            {
                LOG_ERR("on_timeout error\n");
            }
        }
    }while(0);
//    memset(pBase->read_buf_, 0x00, TCP_BUF_LEN);
//    pBase->read_cnt_=0;
    return;
}

int CommTcpBase::close_conn()
{
    if(bev_ != NULL)
    {
    	bufferevent_free(bev_);
    	bev_=NULL;
    }
    if(sock_ >= 0)
    {
    	sock_=-1;
    }
    status_=STAT_UNVALID;
    read_cnt_=0;
    return 0;
}

int CommTcpBase::clear_recvbuf()
{
	memset(read_buf_, 0x00, TCP_BUF_LEN);
	read_cnt_=0;
	return 0;
}

void CommTcpBase::read_cb(struct bufferevent * bev, void * arg)
{
	TRACE;
	int rt=0;
	int isreading=1;
    CommTcpBase * pBase=(CommTcpBase *)arg;
	do{
	    if(bev != pBase->bev_)
        {
        	LOG_ERR("bev != pBase->bev_\n");
        	break;
        }
        int n=0;
        int nread=0;
        while(n=bufferevent_read(bev, pBase->read_buf_+pBase->read_cnt_, TCP_BUF_LEN-pBase->read_cnt_), n>0)
        {
            pBase->read_cnt_+=n;
            nread+=n;
        }
        if(nread==0)
        {
            printf("nread:0, read_cnt_:%d\n", pBase->read_cnt_);
        }
        if(pBase->read_cnt_ >= TCP_BUF_LEN)
        {
        	LOG_ERR("read_buf_ full!");
        	/*TODo:做点什么呢*/
        }
        rt=pBase->on_read_ready(pBase->read_buf_, pBase->read_cnt_);
        if(rt != EC_SUCCESS)
        {
        	LOG_ERR("on_read_ready return error[%d]\n", rt);
        }
        if(pBase->read_done(pBase->read_buf_, pBase->read_cnt_))
        {
        	rt=pBase->handle_pkg(pBase->read_buf_, pBase->read_cnt_);
        	if(rt != EC_SUCCESS)
            {
            	LOG_ERR("handle_pkg return err rt[%d]\n", rt);
            }
            memset(pBase->read_buf_, 0x00, TCP_BUF_LEN);
            pBase->read_cnt_=0;
            isreading=0;
        }

	}while(0);
	if(rt==0 && isreading)
    {
        bufferevent_enable(pBase->bev_, EV_READ);
    }
	return;

}

}//comm_event
}//common
}//dc
