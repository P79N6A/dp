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

class Config : public boost::serialization::singleton<Config> {
public:
    Config() : isparse_(false) {}
    ~Config() {}

    bool IsParse() { return isparse_; }
    int Parse(const std::string & conf_file);

    /* ip address */
    const char * LocalIP();
    int ServerPort();

    /* process config */
    int WorkerCount();
    void ProcessIdx(int pidx);
    int ProcessIdx();

    /* zookeeper config */
    const char * ZKList();

    /* log config */
    const char * LogConf();
    const char * LogCategory();

    /* flow control, token-bucket */
    int FlowMaxToken();
    int FlowCIR();
    int FlowTC();

    /* Agent Conf */
    int AgentClientDataTimeout();

    const char * PidFile();

    void SetForeGround(bool f) { foreground_ = f; }
    bool GetForeGround() { return foreground_; }

private:
    Config(const Config &) {};
    boost::property_tree::ptree pt_;
    std::string conf_file_;
    std::string local_ip_;
    int pidx_;
    bool isparse_;
    bool foreground_;
};

} // mem_sync
} // poseidon
#endif

