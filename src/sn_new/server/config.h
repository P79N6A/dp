/**
 **/

#ifndef  _CONFIG_H_ 
#define  _CONFIG_H_

#include <boost/serialization/singleton.hpp>
#include <boost/property_tree/ptree.hpp>    
#include <boost/property_tree/ini_parser.hpp>    

namespace poseidon
{
namespace sn 
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

    const char * index_data_file();

    const char * index_data_done_file();

    const char * index_data_path();

    void set_index_data_file(const char * filename)
    {
        index_data_file_=filename;
    }

    const char * zk_iplist();
    
    bool ha_on();

    void process_idx(int pidx);

    int process_idx();

    const char * pid_file();

    const char *scoring_file();

    const char *inv_targets();

    int quota(); 

private:
    boost::property_tree::ptree pt_; 
    std::string conf_file_;
    std::string index_data_file_;
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

