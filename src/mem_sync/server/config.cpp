/*
 * Config.cpp
 * Created on: 2016-10-20
 */

#include "config.h"
#include "util/func.h"

namespace poseidon {
namespace mem_sync {

int Config::Parse(const std::string & conf_file)
{
    if (isparse_)
        return 0;

    boost::property_tree::ini_parser::read_ini(conf_file, pt_);
    conf_file_ = conf_file;
    isparse_= true;
    return 0;
}

int Config::ServerPort(void)
{
    return pt_.get<int>("DataSvr.ServerPort", 25600);
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
    return  local_ip_.c_str();
}

int Config::WorkerCount(void)
{
    return pt_.get<int>("DataSvr.WorkerCount", 1);
}

void Config::ProcessIdx(int pidx)
{
    pidx_ = pidx;
}

int Config::ProcessIdx(void)
{
    return pidx_;
}

const char * Config::ZKList(void)
{
    return pt_.get<std::string>("DataSvr.ZKList", "127.0.0.1:2181").c_str();
}

const char * Config::LogConf(void)
{
    return pt_.get<std::string>("DataSvr.LogConf", "").c_str();
}

const char * Config::LogCategory(void)
{
    return pt_.get<std::string>("DataSvr.LogCategory", "datasvr").c_str();
}

int Config::FlowMaxToken(void)
{
	/* default to be 100MB/s */
	return pt_.get<int>("DataSvr.FlowMaxToken", 100 * 1024 * 1024);
}

int Config::FlowCIR(void)
{
	/* default to be 10MB/s */
	return pt_.get<int>("DataSvr.FlowCIR", 10 * 1024 * 1024);
}

int Config::FlowTC(void)
{
	/* Time Interval set to 100ms */
	return pt_.get<int>("DataSvr.FlowTC", 100);
}

int Config::AgentClientDataTimeout(void)
{
	return pt_.get<int>("DataSvr.AgentTimeout", 60);
}

const char * Config::PidFile(void)
{
	return pt_.get<std::string>("DataSvr.PidFile", "../run.pid").c_str();
}

}}


