/**
 **/

#ifndef  _CONFIG_H_ 
#define  _CONFIG_H_

#include <boost/serialization/singleton.hpp>
#include <boost/property_tree/ptree.hpp>    
#include <boost/property_tree/ini_parser.hpp>    

namespace poseidon
{
namespace monitor 
{
class Config:public boost::serialization::singleton<Config>
{
public:
    enum EC
    {
        EC_SUCCESS=0,
        EC_REPARSE,
    };
    Config():isparse_(false){}        
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

    int server_port();

    const char * local_ip();

    int worker_count();

    void process_idx(int pidx);

    int process_idx();

    const char * mysql_host();
    
    const char * mysql_user();

    const char * mysql_pass();
    
    const char * tans_ip();

    int tans_port();

private:
    boost::property_tree::ptree pt_; 
    std::string conf_file_;
    std::string local_ip_;
    int pidx_;
    bool isparse_;
};
}//monitor
}//poseidon
#endif   // ----- #ifndef _CONFIG_H_  ----- 

