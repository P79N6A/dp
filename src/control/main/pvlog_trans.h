/**
 **/

#ifndef  _PVLOG_TRANS_H_ 
#define  _PVLOG_TRANS_H_


#include <boost/serialization/singleton.hpp>
#include "protocol/src/poseidon_proto.h"
#include <netinet/in.h>
#include <arpa/inet.h>

namespace poseidon
{
namespace control 
{
class PvlogTrans:public boost::serialization::singleton<PvlogTrans>
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
    std::string _host;
    int _port;
    int get_log_addr(struct sockaddr_in * addr);
};

}
}

#endif   // ----- #ifndef _PVLOG_TRANS_H_  ----- 

