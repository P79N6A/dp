/**
 * @company             
 */

//include self head file


//include STD C head files
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

//include STD C++ head files


//include third_party_lib head files
#include "comm_event_tcp_connector.h"
#include "comm_event_interface.h"
#include "comm_event_factory.h"

class CTcpTest:public dc::common::comm_event::CommTcpConnector
{

public:

    /** 
     * @brief               ���ӳɹ�ʱ����
     **/
    virtual int on_connected()
    {
        printf("%s called\n", __FUNCTION__);
        char buf[128];
        snprintf(buf, 128, "hello world!\n");
        write(buf, strlen(buf) ); 
        return 0;
    }

    /** 
     * @brief              ��������ʱ����
     * @param
     * @return
     **/
    virtual int on_error()
    {
        printf("%s called\n", __FUNCTION__);
        return 0;
    }
    
    virtual int on_close()
    {
        printf("%s called\n", __FUNCTION__);
        return 0;
    }
    
    virtual int on_timeout()
    {
        printf("%s called\n", __FUNCTION__);
        return 0;
    }

    /** 
     * @brief               ��������ʱ����
     * @return
     **/
    virtual int on_read_ready(const char * buf, const int len)
    {
        printf("%s called. buf[%s]len[%d]\n", __FUNCTION__, buf, len);
        sleep(1);
        write(buf,len);
        return 0;
    }

    /** 
     * @brief               �ж϶����������Ƿ���һ�������İ�
     * @return              true����������handle_pkg��������, false, �����������İ�
     **/
    virtual bool read_done(const char * buf, const int len)
    {
        printf("%s called\n", __FUNCTION__);
        if(len > 10)
        {
            return true;
        }else
        {
            return false;
        }
    }

    
    /** 
     * @brief               ������һ�������İ��ǵ����������
     * @return
     **/
    virtual int handle_pkg(const char * buf, const int len)
    {
        printf("%s called\n", __FUNCTION__);
        return 0;
    }

};

//include project other head files
int main(int argc, char * argv [])
{
    if(dc::common::comm_event::CommFactoryInterface::instance().init() != dc::common::comm_event::EC_SUCCESS)
    {
        printf("CommFactoryInterface::instance().init() return error\n");
    }
    
    CTcpTest * ptcp=new(std::nothrow) CTcpTest();
    if(ptcp == NULL)
    {
        return -1;
    }

    dc::common::comm_event::CommFactoryInterface::instance().add_comm_tcp(ptcp);

    ptcp->init();
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    inet_aton("127.0.0.1", &addr.sin_addr);
    int rt=ptcp->connect(addr);
    if(rt != 0)
    {
    	printf("connect error\n");
    	return -1;
    }

    
    dc::common::comm_event::CommFactoryInterface::instance().run();


}
