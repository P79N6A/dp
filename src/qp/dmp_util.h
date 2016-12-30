/**
 **/
#ifndef  _DMP_UTIL_H_ 
#define  _DMP_UTIL_H_

#include "poseidon_proto.h"

namespace poseidon
{
namespace qp 
{

#define OS_UNKNOWED "0"
#define OS_SYMBIAN "1"
#define OS_ANDROID "2"
#define OS_IPHONE "3"
#define OS_IPAD4 "4"

#define CT_UNKNOWED "0"
#define CT_WIFI "1"
#define CT_2G "2"
#define CT_3G "3"
#define CT_4G "4"

class DmpUtil
{
public:
    static const char * os_to_id(const std::string & str )
    {
        if(str=="ANDROID")
        {
            return OS_ANDROID;
        }else if(str=="IOS")
        {
            return OS_IPHONE;
        }else if(str=="SYMBIAN")
        {
            return OS_SYMBIAN;
        }else
        {
            return OS_UNKNOWED;
        }
    }
    static const char * connection_type_to_id(rtb::ConnectionType t )
    {
        const char * rt=CT_UNKNOWED;
        switch(t)
        {
            case rtb::CONNECTION_TYPE_WIFI:
                rt=CT_WIFI;
                break;
            case rtb::CONNECTION_TYPE_CELLULAR_DATA_2G:
                rt=CT_2G;
                break;
            case rtb::CONNECTION_TYPE_CELLULAR_DATA_3G:
                rt=CT_3G;
                break;
            case rtb::CONNECTION_TYPE_CELLULAR_DATA_4G:
                rt=CT_4G;
                break;
            default:
                rt=CT_UNKNOWED;
                break;
        }
        return rt;
    }

};


}//qp
}//poseidon

#endif   // ----- #ifndef _DMP_UTIL_H_  ----- 


