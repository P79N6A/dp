/**
 **/

#ifndef  _MODEL_UPDATER_BIN_CONFIG_H_ 
#define  _MODEL_UPDATER_BIN_CONFIG_H_

#include <boost/serialization/singleton.hpp>
#include <boost/property_tree/ptree.hpp>    
#include <boost/property_tree/ini_parser.hpp>    

namespace poseidon
{
namespace model_updater
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

    const char * zk_iplist();

    const char * local_ip();
    int ds_port();
private:
    boost::property_tree::ptree pt_; 
    std::string conf_file_;
    bool isparse_;
    std::string local_ip_;
};
}
}
#endif   // _MODEL_UPDATER_BIN_CONFIG_H_

