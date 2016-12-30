/**
 **/

#include "config.h"
#include "util/func.h"

namespace poseidon
{
namespace testtool
{

/**
 * @brief  解析配置文件
 **/
int Config::parse(const std::string & conf_file)
{
    if(isparse_)
    {
        return -1;
    }

    boost::property_tree::ini_parser::read_ini(conf_file, pt_);    
    conf_file_=conf_file;
    isparse_=true;
    return 0;

}

/**
 * @brief       日志的配置文件
 **/

const char * Config::server_ip()
{
    if(local_ip_.empty())
    {
        local_ip_=pt_.get<std::string>("testtool.server_ip","");
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


const char * Config::zk_ip_list()
{
    return pt_.get<std::string>("testtool.zk_ip_list", "").c_str();
}
const char * Config::log_conf()
{
    return pt_.get<std::string>("testtool.log_conf", "").c_str();
}
const char * Config::log_category()
{
    return pt_.get<std::string>("testtool.log_category", "").c_str();
}
int Config::server_port()
{
    return pt_.get<int>("testtool.server_port",400);
}

int Config::data_id()
{
    return pt_.get<int>("testtool.data_id",400);
}

int Config::big_obj_size()
{
    return pt_.get<int>("testtool.big_obj_size",400);
}
int Config::performance_test_time()
{
    return pt_.get<int>("testtool.performance_test_time",400);
}

int Config::performance_test_size()
{
    return pt_.get<int>("testtool.performance_test_size",400);
}



}
}


