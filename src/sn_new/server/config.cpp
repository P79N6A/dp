/**
 **/
#include "config.h"
#include "util/func.h"

namespace poseidon
{
namespace sn 
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
    return pt_.get<std::string>("Sn.LogConf","").c_str();
}

const char * Config::log_category()
{
    return pt_.get<std::string>("Sn.LogCategory","sn").c_str();
}

const char * Config::local_ip()
{
    if(local_ip_.empty())
    {
        local_ip_=pt_.get<std::string>("Sn.LocalIp","");
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
    return pt_.get<int>("Sn.ServerPort",25600);
}


int Config::worker_count()
{
    return pt_.get<int>("Sn.WorkerCount",1);
}


int Config::off_port()
{
    return pt_.get<int>("Sn.OffPort", 1000);
}

const char * Config::index_data_file()
{
    return index_data_file_.c_str();
}

const char * Config::index_data_done_file()
{
    return pt_.get<std::string>("Sn.IndexDataFileDone","../data/index.data.done").c_str();
}

const char * Config::index_data_path()
{
    return pt_.get<std::string>("Sn.IndexDataPath","../data").c_str();
}

const char * Config::zk_iplist()
{
    return pt_.get<std::string>("Sn.ZkIplist", "").c_str();
}

bool Config::ha_on()
{
    if(ha_on_first_)
    {
        if(pt_.get<int>("Sn.HaOn", 0)==0)
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

const char * Config::pid_file()
{
    return pt_.get<std::string>("Sn.PidFile", "./sn.pid").c_str();
}

int Config::quota()
{
    return pt_.get<int>("Sn.Quota", 5);
}

const char *Config::scoring_file()
{
    return pt_.get<std::string>("Sn.ScoringFile", "").c_str();
}

const char *Config::inv_targets()
{
    return pt_.get<std::string>("Sn.InvTargets", "").c_str();
}

}
}


