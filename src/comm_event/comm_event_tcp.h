/**
 * @company             
 */

#ifndef  COMM_EVENT_COMM_EVENT_TCP_H_
#define  COMM_EVENT_COMM_EVENT_TCP_H_

#include <iostream>

extern "C"
{
#include "event2/event.h"
#include "event2/bufferevent.h"
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

class CommTcpBase:public CommTcpInterface
{

public:

    enum
    {
    	STAT_UNVALID=0,     /*��Ч*/
    	STAT_INITED=1,      /*�Ѿ���ʼ��*/
    	STAT_CONNECTING,    /*��������*/
    	STAT_CONNECTED,     /*������*/
    	STAT_CLOSE,         /*�ر�*/
    };
    enum
    {
    	TCP_BUF_LEN=60000,  /*tcp����С*/
    };

    CommTcpBase():base_(NULL),bev_(NULL),sock_(-1),status_(STAT_UNVALID),read_cnt_(0)
    {
    }

    virtual ~CommTcpBase();

    /** 
     * @brief               ���л����ĳ�ʼ��
     * @return              �ɹ�����EC_SUCCESS, ���򷵻ض�Ӧ�Ĵ�����
     **/
    virtual int init(int fd=-1, int status=STAT_INITED);


    /** 
     * @brief               ����״̬
     **/
    int status(){return status_;}

    
    /** 
     * @brief               ���ӳɹ�ʱ����
     **/
    virtual int on_connected();

    /** 
     * @brief              ��������ʱ����
     * @param
     * @return
     **/
    virtual int on_error();
    
    /** 
     * @brief              closeʱ����
     * @param
     * @return
     **/
    virtual int on_close();
    
    /** 
     * @brief              timeoutʱ����
     * @param
     * @return
     **/
    virtual int on_timeout();
    
    /** 
     * @brief               ��������ʱ����
     * @return
     **/
    virtual int on_read_ready(const char * buf, const int len);

    
    /** 
     * @brief               �ж϶����������Ƿ���һ�������İ�
     * @return              true����������handle_pkg��������, false, �����������İ�
     **/
    virtual bool read_done(const char * buf, const int len);

    
    /** 
     * @brief               ������һ�������İ��ǵ����������
     * @return
     **/
    virtual int handle_pkg(const char * buf, const int len) ;


    /** 
     * @brief               
     * @param
     * @return
     **/
    virtual int write(const char * buf, const int len);


    virtual int close_conn();

    int clear_recvbuf();

    /** 
     * @brief               �ѵ�ǰevent�ӵ�event_base����ȥ
     * @param base          [IN]
     * @return              �ɹ�����EC_SUCCESS, ���򷵻ض�Ӧ�Ĵ�����
     **/
    int add_to_base(struct event_base * base);


    static void event_cb(struct bufferevent * bev, short event, void * arg);

    static void read_cb(struct bufferevent * bev, void * arg);


protected:

    void set_status(int status)
    {
    	status_=status;
    }

    struct event_base * base_;          /*base*/
    struct bufferevent * bev_;          /*����tcpͨѶ*/
    int sock_;                          /*sock*/
    int status_;                        /*sock��״̬*/
    int read_cnt_;                       /*read�ֽ���*/ 
    char read_buf_[TCP_BUF_LEN];        /*buf*/

};

}//comm_event
};//common
};//dc

#endif   /* ----- #ifndef COMM_EVENT_COMM_EVENT_TCP_H_  ----- */

