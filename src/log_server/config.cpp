/**
 **/
#include "config.h"
#include "util/func.h"

namespace poseidon
{
namespace log 
{
/**
 * @brief  解析配置文件
 **/
int Config::parse(const std::string & conf_file)
{
    if(isparse_)
    {
        return EC_REPARSE;
    }

    boost::property_tree::ini_parser::read_ini(conf_file, pt_);    
    conf_file_=conf_file;
    isparse_=true;
    return EC_SUCCESS;

}

/**
 * @brief       日志的配置文件
 **/
const char * Config::log_conf()
{
    return pt_.get<std::string>("LogServer.LogConf","").c_str();
}

const char * Config::log_category()
{
    return pt_.get<std::string>("LogServer.LogCategory","logserver").c_str();
}

const char * Config::local_ip()
{
    if(local_ip_.empty())
    {
        local_ip_=pt_.get<std::string>("LogServer.LocalIp","");
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
    return pt_.get<int>("LogServer.ServerPort",25600);
}


int Config::worker_count()
{
    return pt_.get<int>("LogServer.WorkerCount",1);
}


void Config::process_idx(int pidx)
{
    pidx_=pidx;
}

int Config::process_idx()
{
    return pidx_;
}

const char * Config::get_path()
{
    return pt_.get<std::string>("LogServer.Path","/dev/shm/poseidon/pvlog").c_str();
}

const char * Config::get_file_prefix()
{
    return pt_.get<std::string>("LogServer.FilePrefix","pvlog").c_str();
}

int Config::get_rolling_time()
{
    return pt_.get<int>("LogServer.RollTime", 600);
}

const char * Config::get_redis_host()
{
    return pt_.get<std::string>("LogServer.RedisHost","cnode730").c_str();
}

int Config::get_redis_port()
{
    return pt_.get<int>("LogServer.RedisPort", 6384);
}


const char * Config::zk_iplist()
{
    return pt_.get<std::string>("LogServer.ZkIplist", "").c_str();
}

bool Config::ha_on()
{
    if(ha_on_first_)
    {
        if(pt_.get<int>("LogServer.HaOn", 0)==0)
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

const char * Config::pid_file()
{
    return pt_.get<std::string>("LogServer.PidFile", "./log_server.pid").c_str();
}

int Config::quota()
{
    return pt_.get<int>("LogServer.Quota", 3);
}

}
}


