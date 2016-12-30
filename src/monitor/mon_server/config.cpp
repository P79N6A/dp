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
    return pt_.get<std::string>("MonitorSvr.LogConf","").c_str();
}

const char * Config::log_category()
{
    return pt_.get<std::string>("MonitorSvr.LogCategory","monitorsvr").c_str();
}

int Config::server_port()
{
    return pt_.get<int>("MonitorSvr.ServerPort",25600);
}

const char * Config::local_ip()
{
    if(local_ip_.empty())
    {
        local_ip_=pt_.get<std::string>("MonitorSvr.LocalIp","");
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

int Config::worker_count()
{
    return pt_.get<int>("MonitorSvr.WorkerCount",1);
}

void Config::process_idx(int pidx)
{
    pidx_=pidx;
}

int Config::process_idx()
{
    return pidx_;
}

const char * Config::mysql_host()
{
    return pt_.get<std::string>("MonitorSvr.MysqlHost","").c_str();
}

const char * Config::mysql_user()
{
    return pt_.get<std::string>("MonitorSvr.MysqlUser","").c_str();
}

const char * Config::mysql_pass()
{
    return pt_.get<std::string>("MonitorSvr.MysqlPass","").c_str();
}

const char * Config::tans_ip()
{
    return pt_.get<std::string>("MonitorSvr.TansIp","10.32.54.140").c_str();
}

int Config::tans_port()
{
    return pt_.get<int>("MonitorSvr.TansPort", 26200);
}


}
}


