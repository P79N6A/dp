/**
 **/
#ifndef  _FORMAT_FILTER_H_ 
#define  _FORMAT_FILTER_H_
#include <set>
#include <map>
#include <string>
#include <iostream>

namespace poseidon
{
namespace control
{

class FormatFilter
{
public:
    FormatFilter()
    {
        mapformat_["flv"]="video/x-flv";
        mapformat_["mp4"]="video/x-flv";
        mapformat_["jpg"]="image/jpeg";
        mapformat_["png"]="image/png";
    }
    void add_allow_format(const std::string & fmt)
    {
        allow_formats_.insert(fmt);
    }

    bool allow(const std::string & suffix)
    {
        //获取文件后缀
        std::string format=mapformat_[suffix];
        if(allow_formats_.count(format) > 0)
        {
            return true;
        }else
        {
            return false;
        }
    }


private:
    std::set<std::string> allow_formats_;
    std::map<std::string, std::string> mapformat_;//文件后缀-->文件格式
};

}//control
}//poseidon

#endif   // ----- #ifndef _FORMAT_FILTER_H_  ----- 

#if 0
using namespace poseidon::control;
int main()
{
    FormatFilter ff;
    ff.add_allow_format("video/x-flv");
    //true
    bool allow=ff.allow("/root/a.flv");
    //false
    std::cout<<allow<<std::endl;
    allow=ff.allow("/root/a.jpb");
    std::cout<<allow<<std::endl;
    return 0;
}
#endif


