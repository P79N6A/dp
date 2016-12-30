/**
 **/

#ifndef  _UTIL_STR_H_ 
#define  _UTIL_STR_H_
#include <algorithm>
#include <cstdarg>
#include <string>
#include <cstdio>
#include <vector>
#include <map>
#include <boost/algorithm/string.hpp>

namespace poseidon
{
namespace util
{
class UtilStr
{
public:


    UtilStr()
    {
    }

    UtilStr(std::string str)
    {
        str_=str;
    }

    std::string & str()
    {
        return str_;
    }
    
    UtilStr & replace(char oldchar, char newchar)
    {
        std::replace(str_.begin(), str_.end(), oldchar, newchar);
        return *this;
    }

    UtilStr & format(const char * fmt, ...)
    {
        char buf[4096];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, 4096, fmt, ap);
        va_end(ap);
        str_=buf;
        return *this;
    }

    UtilStr & to_upper()
    {
        boost::to_upper(str_);
        return *this;
    }
    UtilStr & to_lower()
    {
        boost::to_lower(str_);
        return *this;

    }
    UtilStr & trim()
    {
        boost::trim(str_);
        return *this;
    }

    UtilStr & trim_right()
    {
        boost::trim_right(str_);
        return *this;
    }

    UtilStr & trim_left()
    {
        boost::trim_left(str_);
        return *this;
    }


    static UtilStr inst(std::string str)
    {
        UtilStr us(str);
        return us;
    }

    std::vector<std::string> & split(const std::string & delim)
    {
        boost::split(vr_, str_, boost::is_any_of(delim));
        return vr_; 
    }

    void split_to_map(const std::string & key_delim, const std::string & val_delim, std::map<std::string, std::string>& output )
    {
        std::vector<std::string> vr;
        boost::split(vr, str_, boost::is_any_of(key_delim));

        std::vector<std::string>::iterator it;
        for(it=vr.begin(); it!=vr.end(); it++)
        {
            size_t pos=it->find(val_delim);
            if(pos==std::string::npos)
            {
                continue;
            }
            std::string key=it->substr(0, pos);
            std::string value=it->substr(pos+1);
            output[key]=value;
        }
    }

private:
    std::string str_;
    std::vector<std::string> vr_;
};

}
}

#endif   // ----- #ifndef _UTIL_STR_H_  ----- 


#if 0
//例子 
    poseidon::util::UtilStr ustr;
    std::cout<<ustr.format(" %s_%s  ","a_b", "c_d").replace('_', 'N').to_lower().to_upper().trim().str()<<std::endl;
#endif

