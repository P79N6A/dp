/**
 **/


#include "pvlog.h"
#include <libgen.h>
#include <unistd.h>
#include <time.h>
#include <log4cpp/Category.hh>
//#include <log4cpp/PropertyConfigurator.hh>
#include <log4cpp/RollingFileAppender.hh>
#include <log4cpp/PatternLayout.hh>

namespace poseidon{
namespace util{

/**
 * @brief  初始化日志
 **/
bool Pvlog::init()
{
    TRACE;
    if(init_)
    {
        fprintf(stderr, "repeat initialization\n");
        return false;
    }
    try
    {
        if(access("/dev/shm/poseidon", F_OK) != 0)
        {
            mkdir("/dev/shm/poseidon", 0777);
        }
        if(access("/dev/shm/poseidon/pvlog", F_OK) != 0)
        {
            mkdir("/dev/shm/poseidon/pvlog", 0777);
        }

        std::string real_name="/dev/shm/poseidon/pvlog/";


        if(name_.empty())
        {
            name_=default_name();
        }
        real_name+=name_;

        log4cpp::RollingFileAppender * appender=new log4cpp::RollingFileAppender("pvlogappender", real_name.c_str(), 10000000, 3);
        log4cpp::PatternLayout * layout=new log4cpp::PatternLayout();
        layout->setConversionPattern("%m");
        appender->setLayout(layout);
        pcategory_=&(log4cpp::Category::getRoot().getInstance("pvlog"));
        pcategory_->addAppender(appender);
        pcategory_->setAdditivity(false);
        pcategory_->setPriority(log4cpp::Priority::INFO);
        init_=true; 
        return true;
    }catch (log4cpp::ConfigureFailure& f)
    {
        fprintf(stderr, "log4cpp parse configure error\n");
        return false;
    }
}

void Pvlog::write(const std::string str)
{
    if(!init_)
    {
        init();
    }
    if(pcategory_ != NULL)
        pcategory_->info("%s\n", str.c_str() );
}
   
}
}

