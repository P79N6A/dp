/**
 **/
#include "config.h"
#include "util/func.h"

namespace poseidon
{
namespace monitor 
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
    return pt_.get<std::string>("Monitor.LogConf","").c_str();
}

const char * Config::log_category()
{
    return pt_.get<std::string>("Monitor.LogCategory","sn").c_str();
}


const char * Config::server_ip()
{
    return pt_.get<std::string>("Monitor.ServerIp","sn").c_str();
}

int Config::server_port()
{
    return pt_.get<int>("Monitor.ServerPort",25600);
}

const char * Config::local_ip()
{
    if(local_ip_.empty())
    {
        local_ip_=pt_.get<std::string>("Monitor.LocalIp","");
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

}
}


