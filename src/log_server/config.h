/**
 **/

#ifndef  _CONFIG_H_ 
#define  _CONFIG_H_

#include <boost/serialization/singleton.hpp>
#include <boost/property_tree/ptree.hpp>    
#include <boost/property_tree/ini_parser.hpp>    

namespace poseidon
{
namespace log 
{
class Config:public boost::serialization::singleton<Config>
{
public:
    enum EC
    {
        EC_SUCCESS=0,
        EC_REPARSE,
    };
    Config():isparse_(false),pidx_(-1),ha_on_(false),ha_on_first_(true){}        
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

    int server_port();

    int worker_count();

    void process_idx(int pidx);

    int process_idx();

    const char * zk_iplist();
    
    bool ha_on();

    const char * get_path();
    const char * get_file_prefix();
    int get_rolling_time();

    const char * get_redis_host();
    int get_redis_port();

    const char * pid_file();

    int quota();

private:
    boost::property_tree::ptree pt_; 
    std::string conf_file_;
    bool isparse_;
    int pidx_;
    bool ha_on_;
    bool ha_on_first_;
    std::string local_ip_;
};
}
}
#endif   // ----- #ifndef _CONFIG_H_  ----- 

