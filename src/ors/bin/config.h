/**
 **/

#ifndef  _CONFIG_H_ 
#define  _CONFIG_H_

#include <boost/serialization/singleton.hpp>
#include <boost/property_tree/ptree.hpp>    
#include <boost/property_tree/ini_parser.hpp>    

namespace poseidon
{
namespace ors 
{
class Config:public boost::serialization::singleton<Config>
{
public:
    enum EC
    {
        EC_SUCCESS=0,
        EC_REPARSE,
    };
    Config():isparse_(false),pidx_(-1),ha_on_first_(true){}        
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

    int off_port();

    const char * algo_conf_path();

    void algo_conf_path(const char * algo_conf_path)
    {
        algo_conf_path_ = algo_conf_path;
    }

    const char * zk_iplist();
    
    int ha_on();

    void process_idx(int pidx);

    int process_idx();

    int stat_on();

    int send_on();

private:
    boost::property_tree::ptree pt_; 
    std::string conf_file_;
    std::string algo_conf_path_;
    int off_port_;      //offset of port per process
    bool isparse_;
    int pidx_;
    int ha_on_;
    int ha_on_first_;
    std::string local_ip_;
    int stat_on_;
    int send_on_;
};
}
}
#endif   // ----- #ifndef _CONFIG_H_  ----- 

