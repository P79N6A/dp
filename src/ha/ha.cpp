/**
 **/

#include "ha.h"
#include "util/func.h"
#include "util/log.h"
#include "util/util_str.h"
#include "attr.h"
#include "monitor_api.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

namespace poseidon
{
namespace ha
{

/**
 * @brief               初始化
 * @param zk_iplist     [IN], zk iplist
 * @return              success return 0, or return other
 **/
int Ha::init(const std::string zk_iplist)
{
//    serial_num_=0;
    int rt=0;
    do{
        rt=Zk4Cpp::init(zk_iplist);
        if(rt != 0)
        {
            break;
        }
        rt=connect();
        if(rt != 0)
        {
            break;
        }
        if(!wait_connect_done())
        {
            rt=-1;
            break;
        }

        srand(time(NULL));

    }while(0);
    return rt;
}


/**
 * @brief               注册服务
 * @param servername    [IN], 服务名
 * @param ip            [IN], IP
 * @param port          [IN], 端口
 * @return              success return 0, or return other
 **/
int Ha::reg_server(const std::string & servername, const std::string & ip, int port, int quota )
{
    int rt=0;
    do{
        if(!isConnected())
        {
            LOG_ERROR("No connect error");
            rt=-1;
            break;
        }
        int rquota=quota;
        if(quota > MAX_QUOTA)
        {
            rquota=MAX_QUOTA;
        }
        if(quota<0)
        {
            rquota=0;
        }

        char path[256];
        int flags=0;

        snprintf(path, 256, "/poseidon/server/%s", servername.c_str());
        createNode(path, NULL, 0, NULL, flags, true);
        memset(path, 0x00, 256);

        snprintf(path, 256, "/poseidon/server/%s/%s:%d:%d", servername.c_str(), ip.c_str(), port, rquota);

//        char data[8];
//        int datalen=0;
        deleteNode(path);//不做返回值判断，不论成功失败

        flags|=ZOO_EPHEMERAL;
        int nrt=createNode(path, NULL, 0, NULL, flags, false);
        if(nrt < 0)
        {
            LOG_ERROR("createNode error");
            rt=-1;
            break;
        }

        RegInfo ri;
        ri.ip=ip;
        ri.quota=rquota;
        ri.port=port;

        map_reg_info_[servername]=ri;

    }while(0);
    return rt;
}


/**
 * @brief               根据servername获取一个地址
 * @param servername    [IN],服务名
 * @param addr          [OUT], 返回地址
 * @return              success return 0, or return other
 **/
int Ha::get_addr(const std::string & servername, struct sockaddr_in & addr  )
{
    int rt=0;
    do{
        //防止多线程情况
        boost::mutex::scoped_lock lock(lock_);
        if(map_serverinfo_.count(servername) == 0)
        {
            rt=update_serverinfo(servername);
            if(rt != 0)
            {
                LOG_ERROR("update_serverinfo error");
                break;
            }
        }
        ServerInfo & si=map_serverinfo_[servername];
        time_t now=time(NULL);
        if(now-si.update_time_>UPDATE_TIME)
        {
            update_serverinfo(servername);
        }
        if(si.addr_list_.size()==0)
        {
            LOG_ERROR("list_size == 0");
            rt=-2;
            break;
        }

        si.serial_num_=(si.serial_num_+1)%si.addr_list_.size();
        addr=si.addr_list_[si.serial_num_];

    }while(0);
    return rt;
}

#if 0
/**
 * @brief               根据servername获取一个地址
 * @param servername    [IN],服务名
 * @param addr          [OUT], 返回地址
 * @return              success return 0, or return other
 **/
int Ha::get_addr_seq(const std::string & servername, struct sockaddr_in & addr  )
{
    int rt=0;
    do{
        //防止多线程情况
        boost::mutex::scoped_lock lock(lock_);
        if(map_serverinfo_.count(servername) == 0)
        {
            rt=update_serverinfo(servername);
            if(rt != 0)
            {
                LOG_ERROR("update_serverinfo error");
                break;
            }
        }
        ServerInfo & si=map_serverinfo_[servername];
        time_t now=time(NULL);
        if(now-si.update_time_>UPDATE_TIME)
        {
            update_serverinfo(servername);
        }
        if(si.addr_list_.size()==0)
        {
            LOG_ERROR("list_size == 0");
            rt=-2;
            break;
        }

        int idx=(serial_num_++)%si.addr_list_.size();
        addr=si.addr_list_[idx];

    }while(0);
    return rt;
}
#endif

void Ha::get_addrEx(const std::string & servername, std::vector<struct sockaddr_in> &vec)
{
      int rt=update_serverinfo(servername);
      if(rt != 0)
      {
          LOG_ERROR("update_serverinfo error");
          return;
      }
      ServerInfo & si=map_serverinfo_[servername];
      vec = si.addr_list_;
}

int Ha::update_serverinfo(const std::string & servername)
{
    int rt=0;
    do{
        if(!isConnected())
        {
            connect();
            if(!wait_connect_done())
            {
                rt=-1;
                break;
            }
        }
        ServerInfo & si=map_serverinfo_[servername];
        si.server_name_=servername;
        si.update_time_=time(NULL);
        
        char path[256];
        snprintf(path, 256, "/poseidon/server/%s", servername.c_str());

        std::list<std::string> childs;      
	    int nrt=getNodeChildren(path, childs);
        if(nrt < 0)
        {
            LOG_ERROR("getNodeChildren");
            break;
        }
        //没有该server对应的服务，则不更新
        if(childs.empty())
        {
            LOG_ERROR("getNodeChildren size=zero ");
            MON_ADD(ATTR_SVR_NOT_ENT, 1);
            rt=-1;
            break;
        }
        
        si.addr_list_.clear();
        std::list<std::string>::iterator it;
        for(it=childs.begin(); it != childs.end(); it++)
        {
            struct sockaddr_in addr;
            //TODO:string->addr
//            char filename[256];
//            snprintf(filename, 256, "%s", it->c_str());
//            std::string base_name=basename(filename);
            std::vector<std::string> vrstr=util::UtilStr::inst(*it).split(":");
        
            if(vrstr.size()<2)
            {
                continue;
            }
            int quota=DEF_QUOTA;
            if(vrstr.size() >= 3)
            {
                quota=util::Func::to_int(vrstr[2]);
            }
            std::string ip=vrstr[0];
            int port=util::Func::to_int(vrstr[1]);
            MakeAddr(addr, ip.c_str(), port);
            for(int i=0; i<quota; i++)
            {
                si.addr_list_.push_back(addr);
            }
        }
//        si.serial_num_=0;

    }while(0);
    return rt;
}

void Ha::onConnect()
{
    LOG_INFO("Ha::onConnect");
    MON_ADD(ATTR_ZK_ON_CONNECT, 1);
}

void Ha::onExpired()
{
    MON_ADD(ATTR_ZK_ON_EXPIRED, 1);
    LOG_ERROR("Ha::onExpired");
    while(1)
    {
        connect();
        if(!wait_connect_done())
        {
            MON_ADD(ATTR_ZK_TRY_CONNECT_FAIL, 1);
            LOG_ERROR("try connect fail...");
            continue;;
        }
        break;
    }
    std::map<std::string, RegInfo >::iterator it;
    for(it = map_reg_info_.begin(); it != map_reg_info_.end(); it++)
    {
        std::string servername=it->first;
        std::string ip=it->second.ip;
        int port=it->second.port;
        int quota=it->second.quota;
        int nrt=reg_server(servername, ip, port, quota);
        if(nrt != 0)
        {
            LOG_ERROR("rereg server error[%s, %s, %d, %d]\n",
                    servername.c_str(), ip.c_str(), port, quota);
        }
    }
    
}

bool Ha::wait_connect_done()
{
    bool rt=true;
    do{
        int max_try_cnt=40;//单次sleep 4 sec
        int i=0;
        while(1)
        {
            //等待链接成功
            if(isConnected())
            {
                break;
            }
            if(++i > max_try_cnt)
            {
                rt=false;
                break;
            }
            usleep(100000);//100ms
        }
    }while(0);
    return rt;
}



}//namespace ha
}//namespace poseidon


