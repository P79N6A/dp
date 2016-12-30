/**
 **/

#include "config.h"
#include "util/func.h"

namespace poseidon
{
namespace dn
{

/**
 * @brief  解析配置文件
 **/
int Config::parse(const std::string & conf_file)
{
    if(isparse_)
    {
        return -1;
    }

    boost::property_tree::ini_parser::read_ini(conf_file, pt_);    
    conf_file_=conf_file;
    isparse_=true;
    return 0;

}

/**
 * @brief       日志的配置文件
 **/
const char * Config::log_conf()
{
    return pt_.get<std::string>("Dn.LogConf","").c_str();
}

const char * Config::log_category()
{
    return pt_.get<std::string>("Dn.LogCategory","dn").c_str();
}

const char * Config::local_ip()
{
    if(local_ip_.empty())
    {
        local_ip_=pt_.get<std::string>("Dn.LocalIp","");
        if(local_ip_=="")
        {
            if(util::Func::get_local_ip(local_ip_)!=0)
            {
                local_ip_="0.0.0.0";
            }
        }
    }
    return  local_ip_.c_str();
}

int Config::server_port()
{
    return pt_.get<int>("Dn.ServerPort",25700);
}

//const char * Config::get_redis_host()
//{
//    return pt_.get<std::string>("Dn.RedisHost","cnode730").c_str();
//}

//int Config::get_redis_port()
//{
//    return pt_.get<int>("Dn.RedisPort", 6379);
//}

int Config::worker_count()
{
    return pt_.get<int>("Dn.WorkerCount",1);
}


const char * Config::zk_iplist()
{
    return pt_.get<std::string>("Dn.ZkIplist", "").c_str();
}

bool Config::ha_on()
{
    if(ha_on_first_)
    {
        if(pt_.get<int>("Dn.HaOn", 0)==0)
        {
            ha_on_=false;
        }else
        {
            ha_on_=true;
        }
        ha_on_first_=false;
    }
    return ha_on_;
}


void Config::process_idx(int pidx)
{
    pidx_=pidx;
}

int Config::process_idx()
{
    return pidx_;
}

//const char * Config::redis_host()
//{
//    return pt_.get<std::string>("Dn.RedisHost", "cnode729").c_str();
//}

//int Config::redis_port()
//{
//    return pt_.get<int>("Dn.RedisPort", 6381);
//}
//int Config::redis_pool_size()
//{
//    return pt_.get<int>("Dn.RedisPoolSize", 500);
////}

bool Config::is_dumb()
{
    return pt_.get<int>("Dn.IsDumb", 0)!=0;
}

}
}


