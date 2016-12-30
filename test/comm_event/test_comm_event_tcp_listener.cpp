/**
 * @company             
 */

//include self head file


//include STD C head files
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

//include STD C++ head files


//include third_party_lib head files
#include "comm_event_tcp_listener.h"
#include "comm_event_tcp.h"
#include "comm_event_factory.h"

class CTcpTest:public dc::common::comm_event::CommTcpBase
{
public:
    /** 
     * @brief               当读到一个完整的包是调用这个函数
     * @return
     **/
    virtual int handle_pkg(const char * buf, const int len)
    {
        printf("%s called\n", __FUNCTION__);
        write(buf, len);
        return 0;
    }

    /** 
     * @brief              发生错误时调用
     * @param
     * @return
     **/
    virtual int on_error()
    {
        printf("%s called\n", __FUNCTION__);
        delete this;
        return 0;
    }
    
    /** 
     * @brief              close时调用
     * @param
     * @return
     **/
    virtual int on_close()
    {
        printf("%s called\n", __FUNCTION__);
        close_conn();
        delete this;
        return 0;
    }

#if 0
    
    /** 
     * @brief              timeout时调用
     * @param
     * @return
     **/
    virtual int on_timeout()
    {
        printf("%s called\n", __FUNCTION__);
        return 0;
    }
    
    /** 
     * @brief               读到数据时调用
     * @return
     **/
    virtual int on_read_ready(const char * buf, const int len)
    {
        printf("%s called\n", __FUNCTION__);
        return 0;
    }

    
    /** 
     * @brief               判断读到的数据是否是一个完整的包
     * @return              true，包完整，handle_pkg将被调用, false, 包不是完整的包
     **/
    virtual bool read_done(const char * buf, const int len)
    {
        printf("%s called\n", __FUNCTION__);
        return true;
    }
#endif

};

class CTcpListenerTest:public dc::common::comm_event::CommTcpListener
{

public:

    int on_accept(evutil_socket_t fd, struct sockaddr *address, int socklen)
    {
        printf("%s called, pid[%d]\n", __FUNCTION__, getpid());
        CTcpTest * ptcp=new CTcpTest();
        dc::common::comm_event::CommFactoryInterface::instance().add_comm_tcp(ptcp);
        ptcp->init(fd, STAT_CONNECTED);
        return 0;
    }

    int on_accept_error()
    {
        printf("%s called\n", __FUNCTION__);
        return 0;
    }
    

};


//include project other head files
int main()
{
    if(dc::common::comm_event::CommFactoryInterface::instance().init() != dc::common::comm_event::EC_SUCCESS)
    {
        printf("CommFactoryInterface::instance().init() return error\n");
    }

    CTcpListenerTest * listener=new(std::nothrow) CTcpListenerTest();
    if(listener == NULL)
    {
    	return -1;

    }
    dc::common::comm_event::CommFactoryInterface::instance().add_comm_tcp(listener);
    listener->init();
    listener->listen_on_addr("127.0.0.1", 8888, 5);

    pid_t pid=fork();
    if(pid==0)
    {
    	if(dc::common::comm_event::CommFactoryInterface::instance().re_init() != 0)
        {
        	printf("re_init error\n");
        	return -1;
        }
    }else if(pid < 0)
    {
    	printf("fork error\n");
    	return -1;
    }
#if 0
    
    dc::common::comm_event::CommTcpInterface * tcpcomm[10];
    for(int i=0; i<10;i++)
    {
        tcpcomm[i]=new(std::nothrow) CTcpTest();
        if(tcpcomm[i] == NULL)
        {
            return -1;
        }

        dc::common::comm_event::CommFactoryInterface::instance().add_comm_tcp(tcpcomm[i]);

        tcpcomm[i]->init();
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(12345);
        inet_aton("10.12.23.99", &addr.sin_addr);
        tcpcomm[i]->connect(addr);
    }
#endif

    
    dc::common::comm_event::CommFactoryInterface::instance().run();


}
