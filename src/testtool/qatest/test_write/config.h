/**
 **/

#ifndef  _CONFIG_H_
#define  _CONFIG_H_

#include <boost/serialization/singleton.hpp>
#include <boost/property_tree/ptree.hpp>    
#include <boost/property_tree/ini_parser.hpp>    

namespace poseidon
{
namespace testtool
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

    const char * server_ip();

    int server_port();

    const char * zk_ip_list();

    int data_id();

    int big_obj_size();

    int performance_test_size();

    int performance_test_time();


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


