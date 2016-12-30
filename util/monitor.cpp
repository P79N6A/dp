/**
 **/

#include "monitor.h"

#include <libgen.h>
#include <unistd.h>
#include <time.h>
#include <log4cpp/Category.hh>
//#include <log4cpp/PropertyConfigurator.hh>
#include <log4cpp/RollingFileAppender.hh>
#include <log4cpp/PatternLayout.hh>


namespace poseidon{
namespace util{

bool Monitor::init()
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
        if(access("/dev/shm/poseidon/monitor", F_OK) != 0)
        {
            mkdir("/dev/shm/poseidon/monitor", 0777);
        }

        std::string real_name="/dev/shm/poseidon/monitor/";


        if(filename_.empty())
        {
            filename_=default_filename();
        }
        real_name+=filename_;

        log4cpp::RollingFileAppender * appender=new log4cpp::RollingFileAppender("monitorappender", real_name.c_str(), 600000000, 10);
        log4cpp::PatternLayout * layout=new log4cpp::PatternLayout();
        layout->setConversionPattern("%m");
        appender->setLayout(layout);
        pcategory_=&(log4cpp::Category::getRoot().getInstance("monitor"));
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


const std::string Monitor::get_basename()
{
    char filename[512];
    memset(filename, 0x00, 512);
    snprintf(filename, 512, "/proc/%d/exe", getpid() );
    char realpath[512];
    memset(realpath, 0x00, 512);
    if(readlink(filename, realpath, 512)>=0)
    {
        return basename(realpath);
    }else
    {
        return "";
    }
}

void Monitor::report(const char * type, int val)
{
    if(!init_)
    {
        init();
    }
    if(host_.empty())
    {
        char hostname[128];
        if(gethostname(hostname, 128) == 0)
        {
            host_=hostname;
        }
    }
    if(service_.empty())
    {
        service_=get_basename();
    }
    int now=time(NULL);
    pcategory_->info("t=%u`type=1`host=%s`service=%s.%s`req=%d\n",
            now, host_.c_str(), service_.c_str(),type,val );
}

void Monitor::report(int type, int val)
{
    if(!init_)
    {
        init();
    }
    if(host_.empty())
    {
        char hostname[128];
        if(gethostname(hostname, 128) == 0)
        {
            host_=hostname;
        }
    }
    if(service_.empty())
    {
        /**/
        service_=get_basename();
    }
    int now=time(NULL);
    pcategory_->info("t=%u`type=1`host=%s`service=%s.%d`req=%d\n",
            now, host_.c_str(), service_.c_str(),type,val );
}


}//util
}//poseidon
