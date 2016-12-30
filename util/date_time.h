/**
**/

#ifndef _UTIL_DATE_TIME_H_
#define _UTIL_DATE_TIME_H_
//include STD C/C++ head files
#include <time.h>    
#include <sys/time.h>    
#include <unistd.h>
#include <stdint.h>

//include third_party_lib head files
#include "third_party/boost/include/boost/serialization/singleton.hpp"

namespace poseidon
{
namespace util
{
class DateTime : public boost::serialization::singleton<DateTime>
{

public:
    DateTime(){}
    virtual ~DateTime(){}

    void Now()
    {
        gettimeofday(&m_timeval, NULL);
        localtime_r(&m_timeval.tv_sec, &m_timeinfo);
    }

    uint64_t GetTimeMs()
    {
        return  m_timeval.tv_sec * 1000 + m_timeval.tv_usec / 1000;
    }

    uint32_t GetUnixTime()
    {
        return m_timeval.tv_sec;
    }

    int GetTimeSecond()
    {
        return  m_timeinfo.tm_sec;
    }

    int GetTimeMinute()
    {
        return m_timeinfo.tm_min; 
    }

    int GetTimeHour()
    {
        return m_timeinfo.tm_hour;
    }

    int GetTimeMDay()
    {
        return m_timeinfo.tm_mday;
    }

    int GetWeekDay()
    {
        return m_timeinfo.tm_wday;
    }

    const char* GetTimeStr(const char * fmt="%Y-%m-%d %H:%M:%S")
    {
         strftime(m_time_str, 32, fmt, &m_timeinfo);
         return m_time_str;
    }

protected:
    timeval m_timeval;
    char m_time_str[32];
    struct tm m_timeinfo;

};
} // namespace util
} // namespace poseidon

#endif // _UTIL_DATE_TIME_H_

