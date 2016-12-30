/**
 */

#ifndef  _COMM_EVENT_H_
#define  _COMM_EVENT_H_

#include <iostream>

extern "C"
{
#include "event2/event.h"
}

#include "comm_event_interface.h"

namespace dc
{
namespace common
{
namespace comm_event
{

/*通讯的工厂类*/
class CommFactory;


/** 
 * @class CommBase
 * @brief CommBase               
 **/
class CommBase:public CommUdpInterface
{
public:

    enum
    {
        MAX_BUF=50000,
    };

    CommBase():listen_event_(NULL),listen_sock_(-1),isbind_(false)
    {
    }

    virtual ~CommBase(){}

    /** 
     * @brief               当收到包时的回调函数
     * @param buf           [IN],收到的buf
     * @param len           [IN],buf的长度
     * @param client_addr   [IN],客户端地址
     * @return              成功返回EC_SUCCESS, 否则返回对应错误码
     **/
    virtual int handle_read(const char * buf, const int len, struct sockaddr_in & client_addr);


    /** 
     * @brief               绑定一个地址
     * @param ip            [IN], ip
     * @param port          [IN], port
     * @return              成功返回EC_SUCCESS, 否则返回对应错误码
     **/
    virtual int bindaddr(const std::string ip, const uint16_t port );
    virtual int bindaddr_nb(const std::string ip, const uint16_t port );

    /** 
     * @brief               往指定ip发送一个包
     * @param buf           [IN]
     * @param len           [IN]
     * @param client_addr   [IN],客户端地址
     * @return              成功返回EC_SUCCESS, 否则返回对应的错误码
     **/
    virtual int send_pkg(const char * buf, const int len, struct sockaddr_in & client_addr );


    /** 
     * @brief               把当前event加到event_base里面去
     * @param base          [IN]
     * @return              成功返回EC_SUCCESS, 否则返回对应的错误码
     **/
    int add_to_base(struct event_base * base);
    
    /** 
     * @brief             回调函数   
     **/
    static void handle_callback(evutil_socket_t listener, short event, void * arg);

private:

    struct event * listen_event_;   /*event*/
    int listen_sock_;               /*sock*/
    std::string ip_;                 
    uint16_t port_; 
    bool isbind_;
};


}//comm_event
};//common
};//dc
#endif   /* ----- #ifndef _COMM_EVENT_H_  ----- */

