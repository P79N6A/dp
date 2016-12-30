/*
 * Config.cpp
 * Created on: 2016-10-20
 */

#include "config.h"
#include <string>
#include "util/func.h"

namespace poseidon {
namespace mem_sync {
namespace agent {

int Config::Parse(const std::string & conf_file)
{
    if (isparse_)
        return 0;

    boost::property_tree::ini_parser::read_ini(conf_file, pt_);
    conf_file_ = conf_file;
    isparse_ = true;
    return 0;
}

const char * Config::LocalIP(void)
{
    if (local_ip_.empty()) {
        local_ip_ = pt_.get<std::string>("DataSvr.LocalIp", "");
        if (local_ip_ == "") {
            if (util::Func::get_local_ip(local_ip_) != 0) {
                local_ip_ = "";
            }
        }
    }
    return local_ip_.c_str();
}

const char * Config::ZKList(void)
{
    return pt_.get<std::string>("DataAgent.ZKList", "127.0.0.1:2181").c_str();
}

const char * Config::LogConf(void)
{
    return pt_.get<std::string>("DataAgent.LogConf", "").c_str();
}

const char * Config::LogCategory(void)
{
    return pt_.get<std::string>("DataAgent.LogCategory", "dataagent").c_str();
}

const char * Config::AgentConf(void)
{
    return pt_.get<std::string>("DataAgent.AgentConf", "").c_str();
}

int Config::AgentClientDataTimeout(void)
{
    return pt_.get<int>("DataAgent.DataTimeout", 300);
}

const char * Config::PidFile(void)
{
    return pt_.get<std::string>("DataAgent.PidFile", "../run.pid").c_str();
}

}  // namespace agent
}  // namespace mem_sync
}  // namespace poseidon

