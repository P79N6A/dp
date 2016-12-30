/**
 */

#ifndef  _COMM_EVENT_H_
#define  _COMM_EVENT_H_

#include <iostream>
#include <queue>

extern "C"
{
#include "event2/event.h"
#include "event2/event_struct.h"
}

#include "comm_event_interface.h"

namespace dc
{
namespace common
{
namespace comm_event
{

/*ͨѶ�Ĺ�����*/
class CommFactory;


/** 
 * @class CommBase_nb
 * @brief CommBase_nb               
 **/
class CommBase_nb:public CommUdpInterface
{
public:

    enum
    {
        MAX_BUF=50000,
    };

    CommBase_nb():listen_sock_(-1),isbind_(false)
    {
    }

    virtual ~CommBase_nb(){}

    /** 
     * @brief               ���յ���ʱ�Ļص�����
     * @param buf           [IN],�յ���buf
     * @param len           [IN],buf�ĳ���
     * @param client_addr   [IN],�ͻ��˵�ַ
     * @return              �ɹ�����EC_SUCCESS, ���򷵻ض�Ӧ������
     **/
    virtual int handle_read(const char * buf, const int len, struct sockaddr_in & client_addr);


    /** 
     * @brief               ��һ����ַ
     * @param ip            [IN], ip
     * @param port          [IN], port
     * @return              �ɹ�����EC_SUCCESS, ���򷵻ض�Ӧ������
     **/
    virtual int bindaddr(const std::string ip, const uint16_t port );


    /** 
     * @brief               ��ָ��ip����һ����
     * @param buf           [IN]
     * @param len           [IN]
     * @param client_addr   [IN],�ͻ��˵�ַ
     * @return              �ɹ�����EC_SUCCESS, ���򷵻ض�Ӧ�Ĵ�����
     **/
    virtual int send_pkg(const char * buf, const int len, struct sockaddr_in & client_addr );


    /** 
     * @brief               �ѵ�ǰevent�ӵ�event_base����ȥ
     * @param base          [IN]
     * @return              �ɹ�����EC_SUCCESS, ���򷵻ض�Ӧ�Ĵ�����
     **/
    int add_to_base(struct event_base * base);
    
    /** 
     * @brief             �ص�����   
     **/
    static void handle_callback(evutil_socket_t listener, short event, void * arg);

private:
    struct event revt;   /*event*/
    struct event wevt;
    int listen_sock_;               /*sock*/
    std::string ip_;                 
    uint16_t port_; 
    bool isbind_;
    typedef struct {
        char *buff;
        int buff_len;
        struct sockaddr_in dst;
    } UdpSegment;
    std::queue<UdpSegment> udp_send_queue_;    
    void OnUdpWrite(evutil_socket_t listener);
};


}//comm_event
};//common
};//dc
#endif   /* ----- #ifndef _COMM_EVENT_H_  ----- */

