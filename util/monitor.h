/**
 **/

#ifndef  _MONITOR_H_ 
#define  _MONITOR_H_



//monitor的具体实现
#include <string>
#include <iostream>
#include <log4cpp/Category.hh>
#include <boost/serialization/singleton.hpp>
#include "util/trace.h"
namespace poseidon{
namespace util{

class Monitor:public boost::serialization::singleton<Monitor>
{
public:
    Monitor():init_(false),pcategory_(NULL)
    {
    }
    
    /**
     * @brief  初始化日志
     **/
    bool init();

    void set_host(const char * host)
    {
        host_=host;
    }

    void set_service(const char * srv_name)
    {
        service_=srv_name;
    }

    void set_filename(const char * filename)
    {
        filename_=filename;
    }

    void report(const char * type, int val);

    void report(int type, int val);
   
private:

    const std::string get_basename();
    const char * default_filename(){return "default";}

private:

    bool init_;
    log4cpp::Category * pcategory_;
    std::string host_;      //host_
    std::string service_;   //service
    std::string filename_;
};


}
}

#define MON_FILE(filename) poseidon::util::Monitor::get_mutable_instance().set_filename(filename)
#define MON_HOST(hostname) poseidon::util::Monitor::get_mutable_instance().set_host(hostname)
#define MON_SERV(srv_name) poseidon::util::Monitor::get_mutable_instance().set_service(srv_name)
//#define MON(type, value) poseidon::util::Monitor::get_mutable_instance().report(type, value)

#define MON(type, value) do{}while(0) 

#endif   // ----- #ifndef _MONITOR_H_  ----- 

