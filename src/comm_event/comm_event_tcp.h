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

/*通讯的工厂类*/
class CommFactory;

class CommTcpBase:public CommTcpInterface
{

public:

    enum
    {
    	STAT_UNVALID=0,     /*无效*/
    	STAT_INITED=1,      /*已经初始化*/
    	STAT_CONNECTING,    /*正在连接*/
    	STAT_CONNECTED,     /*连接上*/
    	STAT_CLOSE,         /*关闭*/
    };
    enum
    {
    	TCP_BUF_LEN=60000,  /*tcp读大小*/
    };

    CommTcpBase():base_(NULL),bev_(NULL),sock_(-1),status_(STAT_UNVALID),read_cnt_(0)
    {
    }

    virtual ~CommTcpBase();

    /** 
     * @brief               进行基本的初始化
     * @return              成功返回EC_SUCCESS, 否则返回对应的错误码
     **/
    virtual int init(int fd=-1, int status=STAT_INITED);


    /** 
     * @brief               返回状态
     **/
    int status(){return status_;}

    
    /** 
     * @brief               连接成功时调用
     **/
    virtual int on_connected();

    /** 
     * @brief              发生错误时调用
     * @param
     * @return
     **/
    virtual int on_error();
    
    /** 
     * @brief              close时调用
     * @param
     * @return
     **/
    virtual int on_close();
    
    /** 
     * @brief              timeout时调用
     * @param
     * @return
     **/
    virtual int on_timeout();
    
    /** 
     * @brief               读到数据时调用
     * @return
     **/
    virtual int on_read_ready(const char * buf, const int len);

    
    /** 
     * @brief               判断读到的数据是否是一个完整的包
     * @return              true，包完整，handle_pkg将被调用, false, 包不是完整的包
     **/
    virtual bool read_done(const char * buf, const int len);

    
    /** 
     * @brief               当读到一个完整的包是调用这个函数
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
     * @brief               把当前event加到event_base里面去
     * @param base          [IN]
     * @return              成功返回EC_SUCCESS, 否则返回对应的错误码
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
    struct bufferevent * bev_;          /*用于tcp通讯*/
    int sock_;                          /*sock*/
    int status_;                        /*sock的状态*/
    int read_cnt_;                       /*read字节数*/ 
    char read_buf_[TCP_BUF_LEN];        /*buf*/

};

}//comm_event
};//common
};//dc

#endif   /* ----- #ifndef COMM_EVENT_COMM_EVENT_TCP_H_  ----- */

