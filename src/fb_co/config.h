/**
 **/

#ifndef  _CONFIG_H_
#define  _CONFIG_H_

#include <boost/serialization/singleton.hpp>
#include <boost/property_tree/ptree.hpp>    
#include <boost/property_tree/ini_parser.hpp>    

namespace poseidon {
namespace feedback {

class Config : public boost::serialization::singleton<Config> {
public:
    Config() : isparse_(false), ha_on_(false), ha_on_first_(true) {}

    int Parse(const std::string & conf);
    bool IsParse(void) { return isparse_; }

    const char * LogConf();
    const char * LogCategory();

    const char * LocalIP();
    int Port();

    const char * ZKList();

    int WorkerCount();
    bool HaOn();

    void ProcessIdx(int pidx);
    int ProcessIdx();

    const char * RedisList();
    int RedisPoolSize();

    bool IsDumb(void);
    const char * PidFile();

    bool GetForeground(void) { return foreground_; }
    void SetForeground(bool fg) { foreground_ = fg; }
private:
    boost::property_tree::ptree pt_; 
    std::string conf_file_;
    int off_port_;      //offset of port per process
    int pidx_;
    std::string local_ip_;

    bool isparse_;
    bool ha_on_;
    bool ha_on_first_;
    bool foreground_;
};

}  // namespace feedback
}  // namespace poseidon

#endif


