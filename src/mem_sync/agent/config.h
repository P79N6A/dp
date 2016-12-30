/*
 * config.h
 * Created on: 2016-10-20
 */

#ifndef  CONFIG_H_
#define  CONFIG_H_

#include <boost/serialization/singleton.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

namespace poseidon {
namespace mem_sync {
namespace agent {

class Config : public boost::serialization::singleton<Config> {
public:
    Config() : isparse_(false), foreground_(false) {}
    ~Config() {}

    bool IsParse() { return isparse_; }
    int Parse(const std::string & conf_file);

    const char * LocalIP();

    /* zookeeper config */
    const char * ZKList();

    /* log config */
    const char * LogConf();
    const char * LogCategory();
    const char * AgentConf();

    /* Agent Conf */
    int AgentClientDataTimeout();

    void SetForeGround(bool f) { foreground_ = f; }
    bool GetForeGround() { return foreground_; }

    const char * PidFile(void);
private:
    Config(const Config &) {};
    boost::property_tree::ptree pt_;
    std::string conf_file_;
    std::string local_ip_;
    bool isparse_;
    bool foreground_;
};

} // mem_sync
} // poseidon
} // agent
#endif

