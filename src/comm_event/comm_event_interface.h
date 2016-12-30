/**
 */

#ifndef  _COMM_EVENT_INTERFACE_H_
#define  _COMM_EVENT_INTERFACE_H_
#include <string>
#include <iostream>
#include <stdint.h>
#include "arpa/inet.h"
#include "sys/socket.h"
#include "netinet/in.h"

namespace dc 
{
namespace common
{
namespace comm_event
{


/** 
 * @brief               错误码
 **/
enum EC
{
    EC_SUCCESS=0,
    EC_PARAM_ERROR,
    EC_UNBIND_ERROR,
    EC_SOCKET_ERROR,
    EC_BIND_ERROR,
    EC_SEND_ERROR,
    EC_NEW_BASE_ERROR,
    EC_STAT_ERR,
    EC_NEW_SOCKET_ERR,
    EC_CONNECT_ERR,
    EC_BASE_NULL,
    EC_EVTIME_NEW_ERR,
};


/** 
 * @brief               一个通讯实例，对应一个绑定的ip、端口
 * @param
 * @return
 **/
class CommUdpInterface
{
public:

    CommUdpInterface(){}

    virtual ~CommUdpInterface(){}



    /** 
     * @brief               当收到包时的回调函数
     * @param buf           [IN],收到的buf
     * @param len           [IN],buf的长度
     * @param client_addr   [IN],客户端地址
     * @return              成功返回EC_SUCCESS, 否则返回对应错误码
     **/
    virtual int handle_read(const char * buf, const int len, struct sockaddr_in & client_addr )=0;


    /** 
     * @brief               绑定一个地址
     * @param ip            [IN], ip
     * @param port          [IN], port
     * @return              成功返回EC_SUCCESS, 否则返回对应错误码
     **/
    virtual int  bindaddr(const std::string ip, const uint16_t port )=0;
    virtual int  bindaddr_nb(const std::string ip, const uint16_t port )=0;

    /** 
     * @brief               往指定ip发送一个包
     * @return              成功返回EC_SUCCESS, 否则返回对应的错误码
     **/
    virtual int send_pkg(const char * buf, const int len, struct sockaddr_in & client_addr)=0;
};

class CommTcpInterface
{
public:
    CommTcpInterface() {}
    virtual ~CommTcpInterface(){}

    /** 
     * @brief               进行基本的初始化
     * @return              成功返回EC_SUCCESS, 否则返回对应的错误码
     **/
    virtual int init(int fd, int status)=0;

    
    /** 
     * @brief               连接成功时调用
     **/
    virtual int on_connected()=0;


    /** 
     * @brief              发生错误时调用
     * @param
     * @return
     **/
    virtual int on_error()=0;
    
    /** 
     * @brief              close时调用
     * @param
     * @return
     **/
    virtual int on_close()=0;
    
    /** 
     * @brief              timeout时调用
     * @param
     * @return
     **/
    virtual int on_timeout()=0;
    
    /** 
     * @brief               读到数据时调用
     * @return
     **/
    virtual int on_read_ready(const char * buf, const int len)=0;

    
    /** 
     * @brief               判断读到的数据是否是一个完整的包
     * @return              true，包完整，handle_pkg将被调用, false, 包不是完整的包
     **/
    virtual bool read_done(const char * buf, const int len)=0;

    
    /** 
     * @brief               当读到一个完整的包是调用这个函数
     * @return
     **/
    virtual int handle_pkg(const char * buf, const int len)=0 ;


    
    /** 
     * @brief               
     * @param
     * @return
     **/
    virtual int write(const char * buf, const int len)=0;


};

/** 
 * @class CommTimerInterface
 * @brief CommTimerInterface
 **/
class CommTimerInterface
{

public:
    CommTimerInterface() {}
    virtual ~CommTimerInterface(){}


    /** 
     * @brief               set timeout time(ms)
     * @param
     * @return
     **/
    virtual int set_timeout(int timeout)=0;


    /** 
     * @brief               session timeout callback
     * @param
     * @return
     **/
    virtual int on_timeout()=0;
};

class CommRedisInterface
{
public:
    CommRedisInterface(){}
    virtual ~CommRedisInterface(){}

    virtual int connect(const std::string & host, int port)=0;

    virtual void disconnect()=0;

    virtual int send_cmd(void * sess_data, const char * fmt, ...) = 0;

    virtual int handle_get_result(void * sess_data, void * pRedisReply)=0;   

};


/** 
 * @class CommFactoryInterface
 * @brief CommFactoryInterface
 **/
class CommFactoryInterface
{
public:
    
    /** 
     * @brief               进行基本的初始化
     * @return              成功返回EC_SUCCESS, 否则返回对应的错误码
     **/
    virtual int init()=0;

    /** 
     * @brief               添加一个comm实例
     * @param comm          [IN],通讯类
     * @return              成功返回EC_SUCCESS, 否则返回对应的错误码
     **/
    virtual int add_comm(CommUdpInterface * comm)=0;

    virtual int add_comm_tcp(CommTcpInterface * tcpcomm)=0;

    virtual int add_comm_timer(CommTimerInterface * timercomm)=0;
    
    virtual int add_comm_redis(CommRedisInterface * rediscomm)=0;


    /**
     * @brief               fork之后, 由子进程调用必须调用
     * @return              成功返回0, 否则返回-1
     */
    virtual int re_init()=0;

    /** 
     * @brief               开始跑
     * @return              成功返回EC_SUCCESS, 否则返回对应的错误码
     **/
    virtual int run()=0;

    /** 
     * @brief               返回一个CommFactoryInterface实例
     * @return
     **/
    static CommFactoryInterface & instance();

};



}//comm_event
}//common
}//dc
#endif   /* ----- #ifndef _COMM_INTERFACE_H_  ----- */

