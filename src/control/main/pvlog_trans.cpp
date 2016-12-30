/**
 **/


#include "pvlog_trans.h"

#include <sys/types.h> 
#include <sys/socket.h> 
#include <errno.h>
#include "util/func.h" 
#include "util/log.h" 
#include "../main/config.h"
#include "ha.h"

namespace poseidon
{
namespace control 
{

/**
 * @brief               初始化
 **/
int PvlogTrans::init(const char * ip, int port)
{
  sock_=0;
  _host=ip;
  _port=port;
    return 0;
}

/**
 * @brief               转发包
 **/
int PvlogTrans::trans(const char * buf, int len)
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
        struct sockaddr_in addr;
        rt=get_log_addr(&addr);
        if(rt!=0)
        {
            LOG_ERROR("get_log_addr error");
            rt=-2;
            break;
        }
        int nsend=sendto(sock_, buf, len, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in) );
        if(nsend <= 0)
        {
            LOG_ERROR("sendto error[%s]", strerror(errno));
            rt=-1;
            break;
        }
        
    }while(0);
    return rt;
}

int PvlogTrans::get_log_addr(struct sockaddr_in * addr)
{
    int rt=0;
    do{
        if(Config::get_mutable_instance().ha_on())
        {
            rt=HA_GET_ADDR("log_server", (*addr) );
            if(rt != 0)
            {
                LOG_ERROR("HA_GET_ADDR error");
                break;
            }
            LOG_DEBUG("qp addr[%s]", util::Func::to_str(*addr).c_str());
        }else
        {
            MakeAddr((*addr), _host.c_str(), _port);
        }
    }while(0);
    return rt;
}

}
}

