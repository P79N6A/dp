/**
 **/

#ifndef  _CONFIG_H_
#define  _CONFIG_H_

#include <boost/serialization/singleton.hpp>
#include <boost/property_tree/ptree.hpp>    
#include <boost/property_tree/ini_parser.hpp>    

namespace poseidon
{
namespace qp
{
class Config:public boost::serialization::singleton<Config>
{
public:
    Config():isparse_(false),ha_on_(false),ha_on_first_(true)
    {
    }

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


    const char * zk_iplist();
    
    bool ha_on();

    void process_idx(int pidx);

    int process_idx();

    const char * redis_host();

    int redis_port();

    int redis_pool_size();

    bool is_dumb();

    const char * pid_file();

private:
    boost::property_tree::ptree pt_; 
    std::string conf_file_;
    int off_port_;      //offset of port per process
    bool isparse_;
    int pidx_;
    bool ha_on_;
    bool ha_on_first_;
    std::string local_ip_;
};
}
}

#endif   // ----- #ifndef _CONFIG_H_  ----- 


