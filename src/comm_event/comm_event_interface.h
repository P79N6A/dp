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
 * @brief               ������
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
 * @brief               һ��ͨѶʵ������Ӧһ���󶨵�ip���˿�
 * @param
 * @return
 **/
class CommUdpInterface
{
public:

    CommUdpInterface(){}

    virtual ~CommUdpInterface(){}



    /** 
     * @brief               ���յ���ʱ�Ļص�����
     * @param buf           [IN],�յ���buf
     * @param len           [IN],buf�ĳ���
     * @param client_addr   [IN],�ͻ��˵�ַ
     * @return              �ɹ�����EC_SUCCESS, ���򷵻ض�Ӧ������
     **/
    virtual int handle_read(const char * buf, const int len, struct sockaddr_in & client_addr )=0;


    /** 
     * @brief               ��һ����ַ
     * @param ip            [IN], ip
     * @param port          [IN], port
     * @return              �ɹ�����EC_SUCCESS, ���򷵻ض�Ӧ������
     **/
    virtual int  bindaddr(const std::string ip, const uint16_t port )=0;
    virtual int  bindaddr_nb(const std::string ip, const uint16_t port )=0;

    /** 
     * @brief               ��ָ��ip����һ����
     * @return              �ɹ�����EC_SUCCESS, ���򷵻ض�Ӧ�Ĵ�����
     **/
    virtual int send_pkg(const char * buf, const int len, struct sockaddr_in & client_addr)=0;
};

class CommTcpInterface
{
public:
    CommTcpInterface() {}
    virtual ~CommTcpInterface(){}

    /** 
     * @brief               ���л����ĳ�ʼ��
     * @return              �ɹ�����EC_SUCCESS, ���򷵻ض�Ӧ�Ĵ�����
     **/
    virtual int init(int fd, int status)=0;

    
    /** 
     * @brief               ���ӳɹ�ʱ����
     **/
    virtual int on_connected()=0;


    /** 
     * @brief              ��������ʱ����
     * @param
     * @return
     **/
    virtual int on_error()=0;
    
    /** 
     * @brief              closeʱ����
     * @param
     * @return
     **/
    virtual int on_close()=0;
    
    /** 
     * @brief              timeoutʱ����
     * @param
     * @return
     **/
    virtual int on_timeout()=0;
    
    /** 
     * @brief               ��������ʱ����
     * @return
     **/
    virtual int on_read_ready(const char * buf, const int len)=0;

    
    /** 
     * @brief               �ж϶����������Ƿ���һ�������İ�
     * @return              true����������handle_pkg��������, false, �����������İ�
     **/
    virtual bool read_done(const char * buf, const int len)=0;

    
    /** 
     * @brief               ������һ�������İ��ǵ����������
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
     * @brief               ���л����ĳ�ʼ��
     * @return              �ɹ�����EC_SUCCESS, ���򷵻ض�Ӧ�Ĵ�����
     **/
    virtual int init()=0;

    /** 
     * @brief               ���һ��commʵ��
     * @param comm          [IN],ͨѶ��
     * @return              �ɹ�����EC_SUCCESS, ���򷵻ض�Ӧ�Ĵ�����
     **/
    virtual int add_comm(CommUdpInterface * comm)=0;

    virtual int add_comm_tcp(CommTcpInterface * tcpcomm)=0;

    virtual int add_comm_timer(CommTimerInterface * timercomm)=0;
    
    virtual int add_comm_redis(CommRedisInterface * rediscomm)=0;


    /**
     * @brief               fork֮��, ���ӽ��̵��ñ������
     * @return              �ɹ�����0, ���򷵻�-1
     */
    virtual int re_init()=0;

    /** 
     * @brief               ��ʼ��
     * @return              �ɹ�����EC_SUCCESS, ���򷵻ض�Ӧ�Ĵ�����
     **/
    virtual int run()=0;

    /** 
     * @brief               ����һ��CommFactoryInterfaceʵ��
     * @return
     **/
    static CommFactoryInterface & instance();

};



}//comm_event
}//common
}//dc
#endif   /* ----- #ifndef _COMM_INTERFACE_H_  ----- */

