/**
 **/

#include "config.h"
#include "util/func.h"

namespace poseidon {
namespace feedback {

int Config::Parse(const std::string &conf)
{
    if (isparse_) {
        return -1;
    }

    boost::property_tree::ini_parser::read_ini(conf, pt_);
    conf_file_ = conf;
    isparse_ = true;
    return 0;
}

const char *Config::LogConf()
{
    return pt_.get<std::string>("FB.LogConf", "").c_str();
}

const char *Config::LogCategory()
{
    return pt_.get<std::string>("FB.LogCategory", "fb").c_str();
}

const char *Config::LocalIP()
{
    if (local_ip_.empty()) {
        local_ip_ = pt_.get<std::string>("FB.LocalIp", "");
        if (local_ip_ == "") {
            if (util::Func::get_local_ip(local_ip_) != 0) {
                local_ip_ = "0.0.0.0";
            }
        }
    }
    return local_ip_.c_str();
}

int Config::Port()
{
    return pt_.get<int>("FB.Port", 25700);
}

int Config::WorkerCount()
{
    /* let worker be the number of cpu cores. */
    // return pt_.get<int>("FB.WorkerCount", sysconf(_SC_NPROCESSORS_CONF));
    return pt_.get<int>("FB.WorkerCount", 1);
}

const char *Config::ZKList()
{
    return pt_.get<std::string>("FB.ZKList", "127.0.0.1:2181").c_str();
}

bool Config::HaOn()
{
    if (ha_on_first_) {
        if (pt_.get<int>("FB.HaOn", 0) == 0) {
            ha_on_ = false;
        } else {
            ha_on_ = true;
        }
        ha_on_first_ = false;
    }
    return ha_on_;
}

void Config::ProcessIdx(int pidx)
{
    pidx_ = pidx;
}

int Config::ProcessIdx()
{
    return pidx_;
}

const char *Config::RedisList()
{
    return pt_.get<std::string>("FB.RedisList", "127.0.0.1:6379").c_str();
}

int Config::RedisPoolSize()
{
    return pt_.get<int>("FB.RedisPoolSize", 10);
}

bool Config::IsDumb()
{
    return pt_.get<int>("FB.IsDumb", 0) != 0;
}

const char *Config::PidFile()
{
    return pt_.get<std::string>("FB.PidFile", "../run.pid").c_str();
}
}
}
