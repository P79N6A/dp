/**
 **/

#ifndef  _CONFIG_H_ 
#define  _CONFIG_H_

#include <boost/serialization/singleton.hpp>
#include <boost/property_tree/ptree.hpp>    
#include <boost/property_tree/ini_parser.hpp>    
#include <set>

namespace poseidon
{
namespace control
{
class Config:public boost::serialization::singleton<Config>
{
public:
    enum EC
    {
        EC_SUCCESS=0,
        EC_REPARSE,
    };
    Config():isparse_(false),pidx_(-1),ha_on_first_(true),filter_ts_flag_(false){}        
    ~Config(){}


    /**
     * @brief  解析配置文件
     **/
    int parse(const std::string & conf_file);

    /**
     * @brief       日志的配置文件
     **/
    const char * log_conf();

    const char * log_category();

    const char * local_ip();

    int ha_on();

    int adapter_port();

    int qp_port();

    int sn_port();

    int dn_port();

    int ors_port();

    int fc_port();

    int fb_port();

    int fc_on();

    const char * zk_iplist();

    int worker_count();


    int max_session();

    int session_timeout();

    int off_port();

    const char * dspid();


    void process_idx(int pidx);

    int process_idx();
    
    int mem_sync_shm_key();


    /**
     * @brief       返回流量是否不竞价
     **/
    bool no_bid(int traffic_source);


    /**
     * @brief       返回流量是否只竞价1分钱
     **/
    bool bid_one_cent(int traffic_source);

    int pv_trans_port();
    const char * pv_trans_ip();

    int max_qps_per_proc();
    
    const char * yunos_mapped_path();
		
private:
    boost::property_tree::ptree pt_; 
    std::string conf_file_;
    int off_port_;      //offset of port per process
    bool isparse_;
    int pidx_;
    int ha_on_;
    bool ha_on_first_;
    std::string local_ip_;
    bool filter_ts_flag_;
    std::string filter_ts_string_;
    std::set<int> set_no_bid_;
    std::set<int> set_bid_one_cent_;
		std::set<int> set_copy_specific_data_;

};
}
}
#endif   // ----- #ifndef _CONFIG_H_  ----- 

