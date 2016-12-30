/**
 **/


#include "monitor_tans.h"

#include <sys/types.h> 
#include <sys/socket.h> 
#include <errno.h>
#include "util/func.h" 
#include "util/log.h" 

namespace poseidon
{
namespace monitor
{

/**
 * @brief               初始化
 **/
int MonitorTans::init(const char * ip, int port)
{
    MakeAddr(addr_, ip, port);
    return 0;
}

/**
 * @brief               转发包
 **/
int MonitorTans::trans(const char * buf, int len)
{
    int rt=0;
    do{
        if(sock_ <= 0)
        {
            sock_=socket(AF_INET, SOCK_DGRAM, 0);
            if(sock_ < 0)
            {
                LOG_ERROR("socket error[%s]\n", strerror(errno));
                rt=-1;
                break;
            }
        }
        int nsend=sendto(sock_, buf, len, 0, (struct sockaddr *)&addr_, sizeof(struct sockaddr_in) );
        if(nsend <= 0)
        {
            LOG_ERROR("sendto error[%s]", strerror(errno));
            rt=-1;
            break;
        }
        
    }while(0);
    return rt;
}

}
}

