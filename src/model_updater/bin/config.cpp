/**
 **/
#include "src/model_updater/bin/config.h"
#include "util/func.h"

namespace poseidon
{
namespace model_updater 
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
    return pt_.get<std::string>("ModelUpdater.LogConf","").c_str();
}

const char * Config::log_category()
{
    return pt_.get<std::string>("ModelUpdater.LogCategory","").c_str();
}

const char* Config::zk_iplist()
{
    return pt_.get<std::string>("ModelUpdater.ZkIplist", "").c_str();
}

int Config::ds_port()
{
    return pt_.get<int>("ModelUpdater.DsPort", 0);
}

const char * Config::local_ip()
{
    if(local_ip_.empty())
    {
        local_ip_=pt_.get<std::string>("ModelUpdater.LocalIp","");
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


