/**
 **/

#ifndef  _LOG_H_ 
#define  _LOG_H_
#include <string>
#include <iostream>
#include <log4cpp/Category.hh>
#include <log4cpp/PropertyConfigurator.hh>
#include <log4cpp/OstreamAppender.hh>
#include <boost/serialization/singleton.hpp>
#include "util/trace.h"
namespace poseidon{
namespace util{

class Log:public boost::serialization::singleton<Log>
{
public:
    Log():init_(false),pcategory_(NULL)
    {
    }
    
    /**
     * @brief  初始化日志
     **/
    bool init(const std::string conffile, const std::string category);

    
    /**
     * @brief  返回log4cpp的category
     **/
    log4cpp::Category & category();

private:

    bool init_;
    log4cpp::Category * pcategory_;
};


}
}

//初始化日志
#define LOG_INIT(conffile, category) poseidon::util::Log::get_mutable_instance().init(conffile, category)

//打印日志的宏，使用参见test/log
//因为log4cpp不支持打印行号、文件名等信息，这里用宏实现
//注意，LOG_XXXX(fmt, ...)这里的fmt不能是变量，必须是字符串,
//因为变量不支持默认连接操作,
//所以如果
//char * str="I am a str";
//LOG_DEBUG(str);    //error, because debug("%d in %s"str);编译不过去
//解决办法==>LOG_DEBUG("I am a str") or LOG_DEBUG("%s", str)
//
//#define LOG_DEBUG(fmt, a...) poseidon::util::Log::get_mutable_instance().category().debug("[%d in %s]"fmt, __LINE__, __FILE__, ##a)
//#define LOG_INFO(fmt, a...) poseidon::util::Log::get_mutable_instance().category().info("[%d in %s]"fmt, __LINE__, __FILE__, ##a)
#define LOG_NOTICE(fmt, a...) poseidon::util::Log::get_mutable_instance().category().notice("[%d in %s]"fmt, __LINE__, __FILE__, ##a)
#define LOG_WARN(fmt, a...) poseidon::util::Log::get_mutable_instance().category().warn("[%d in %s]"fmt, __LINE__, __FILE__, ##a)
#define LOG_ERROR(fmt, a...) poseidon::util::Log::get_mutable_instance().category().error("[%d in %s]"fmt, __LINE__, __FILE__, ##a)
#define LOG_FATAL(fmt, a...) poseidon::util::Log::get_mutable_instance().category().fatal("[%d in %s]"fmt, __LINE__, __FILE__, ##a)

#define LOG_DEBUG(fmt, a...) if(poseidon::util::Log::get_mutable_instance().category().isDebugEnabled())poseidon::util::Log::get_mutable_instance().category().debug("[%d in %s]"fmt, __LINE__, __FILE__, ##a)
#define LOG_INFO(fmt, a...) if(poseidon::util::Log::get_mutable_instance().category().isInfoEnabled())poseidon::util::Log::get_mutable_instance().category().info("[%d in %s]"fmt, __LINE__, __FILE__, ##a)
#endif   // ----- #ifndef _LOG_H_  ----- 

