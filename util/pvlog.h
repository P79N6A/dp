/**
 **/

#ifndef  _PVLOG_H_ 
#define  _PVLOG_H_

//monitor的具体实现
#include <string>
#include <iostream>
#include <log4cpp/Category.hh>
#include <boost/serialization/singleton.hpp>
#include "util/trace.h"
namespace poseidon{
namespace util{

class Pvlog:public boost::serialization::singleton<Pvlog>
{
public:
    Pvlog():init_(false),pcategory_(NULL)
    {
    }
    
    /**
     * @brief  初始化日志
     **/
    bool init();

    void set_name(const char * name)
    {
        name_=name;
    }

    void write(const std::string str);
   
private:

    const char * default_name(){return "default";}

private:

    bool init_;
    log4cpp::Category * pcategory_;
    std::string name_;
};


}
}

#define PV_NAME(filename) poseidon::util::Pvlog::get_mutable_instance().set_name(filename)
#define PVLOG(content) poseidon::util::Pvlog::get_mutable_instance().write(content)

#endif   // ----- #ifndef _PVLOG_H_  ----- 


