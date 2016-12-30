/**
 **/

#ifndef  _MONITOR_TANS_H_ 
#define  _MONITOR_TANS_H_

#include <boost/serialization/singleton.hpp>
#include "protocol/src/poseidon_proto.h"
#include <netinet/in.h>
#include <arpa/inet.h>

namespace poseidon
{
namespace monitor
{
class MonitorTans:public boost::serialization::singleton<MonitorTans>
{
public:


    /**
     * @brief               初始化
     **/
    int init(const char * ip, int port);

    /**
     * @brief               转发包
     **/
    int trans(const char * buf, int len);

private:

    int sock_;
    struct sockaddr_in addr_;
};

}
}

#endif   // ----- #ifndef _MONITOR_TANS_H_  ----- 

