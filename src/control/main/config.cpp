/**
 **/
#include "config.h"

#include "util/func.h"
#include "json/json.h"

namespace poseidon
{
namespace control
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
    
    filter_ts_string_=pt_.get<std::string>("Control.FilterTs", "");
    if(!filter_ts_string_.empty())
    {
        Json::Reader json_read; 
        Json::Value json_filter;
        if( !json_read.parse(filter_ts_string_, json_filter, false) )
        {
            fprintf(stderr, "parse Control.filter_ts error[%s]", filter_ts_string_.c_str());
        }else
        {
            if(json_filter["no_bid"].isArray())
            {
                int size=json_filter["no_bid"].size();
                for(int i=0; i < size; i++)
                {
                    set_no_bid_.insert(json_filter["no_bid"][i].asInt());
                    fprintf(stderr, "WARN: set traffic_source[%d] no bid\n", json_filter["no_bid"][i].asInt() );
                }
            }
            if(json_filter["bid_one_cent"].isArray())
            {
                int size=json_filter["bid_one_cent"].size();
                for(int i=0; i < size; i++)
                {
                    set_bid_one_cent_.insert(json_filter["bid_one_cent"][i].asInt());
                    fprintf(stderr, "WARN: set traffic_source[%d] bid_one_cent\n", json_filter["bid_one_cent"][i].asInt() );
                }
            }

            filter_ts_flag_=true;
        }
    }
    isparse_=true;
    return EC_SUCCESS;

}

/**
 * @brief       日志的配置文件
 **/
const char * Config::log_conf()
{
    return pt_.get<std::string>("Control.LogConf","").c_str();
}

const char * Config::log_category()
{
    return pt_.get<std::string>("Control.LogCategory","control").c_str();
}

const char * Config::local_ip()
{
    if(local_ip_.empty())
    {
        local_ip_=pt_.get<std::string>("Control.LocalIp","");
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

int Config::adapter_port()
{
    return pt_.get<int>("Control.AdapterPort",25600);
}

int Config::qp_port()
{
    return pt_.get<int>("Control.QpPort",25601);
}

int Config::sn_port()
{
    return pt_.get<int>("Control.SnPort",25602);
}

int Config::dn_port()
{
    return pt_.get<int>("Control.DnPort",25603);
}

int Config::ors_port()
{
    return pt_.get<int>("Control.OrsPort",25604);
}

int Config::fc_port()
{
    return pt_.get<int>("Control.FcPort",25605);
}

int Config::fb_port()
{
    return pt_.get<int>("Control.FbPort",25606);
}

int Config::fc_on()
{
    return pt_.get<int>("Control.FcOn", 1);
}

int Config::worker_count()
{
    return pt_.get<int>("Control.WorkerCount",1);
}

const char * Config::dspid()
{
    return pt_.get<std::string>("Control.Dspid", "").c_str();
}

int Config::max_session()
{
    return pt_.get<int>("Control.MaxSession", 10000);
}

int Config::session_timeout()
{
    return pt_.get<int>("Control.SessionTimeout",100);
}


int Config::off_port()
{
    return pt_.get<int>("Control.OffPort", 100);
}

int Config::mem_sync_shm_key()
{
  return pt_.get<int>("Control.MemSyncShmKey", 0);
}

void Config::process_idx(int pidx)
{
    pidx_=pidx;
}

int Config::process_idx()
{
    return pidx_;
}

int Config::ha_on()
{
    if(ha_on_first_)
    {
        if(pt_.get<int>("Control.HaOn", 0)==0)
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

const char * Config::zk_iplist()
{
    return pt_.get<std::string>("Control.ZkIplist", "").c_str();
}

int Config::pv_trans_port()
{
    return pt_.get<int>("Control.PvTransPort", 49999);
}

const char * Config::pv_trans_ip()
{
    return pt_.get<std::string>("Control.PvTransIp", "127.0.0.1").c_str();
}

int Config::max_qps_per_proc()
{
    return pt_.get<int>("Control.MaxQpsPerProc", -1);
}

/**
 * @brief       返回流量是否不竞价
 **/
bool Config::no_bid(int traffic_source)
{
    bool rt=false;
    do{
        if(!filter_ts_flag_)
        {
            break;
        }
        rt=(set_no_bid_.count(traffic_source) > 0);
    }while(0);
    return rt;
}


/**
 * @brief       返回流量是否只竞价1分钱
 **/
bool Config::bid_one_cent(int traffic_source)
{
    bool rt=false;
    do{
        if(!filter_ts_flag_)
        {
            break;
        }
        rt=(set_bid_one_cent_.count(traffic_source) > 0);
    }while(0);
    return rt;
}

const char * Config::yunos_mapped_path()
{
    return pt_.get<std::string>("Control.YunosMappedPath","../data/yunos_game.dict").c_str();
}

}
}


