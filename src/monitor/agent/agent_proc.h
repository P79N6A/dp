/**
 **/

#ifndef  _AGENT_PROC_H_ 
#define  _AGENT_PROC_H_

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <boost/serialization/singleton.hpp>
#include "protocol/src/poseidon_proto.h"
#include "../common/define.h"

namespace poseidon
{
namespace monitor
{

class AgentProc:public boost::serialization::singleton<AgentProc>
{
public:

    AgentProc():layout_(NULL), sock_(-1){}

    int init();

    int run();

    int report(ReportInfoReq & reportinfo); 

    int send_report(std::string &sendbuf);

    int connect_server(); 

    int collect_data(int time_min, ReportInfoReq & reportinfo);

private:

    MonitorLayOut * layout_;        //内存布局
    int sock_;                      //用来于monitor_server通讯的sock_;
    struct sockaddr_in serv_addr_;  //server 地址

};

}
}

#endif   // ----- #ifndef _AGENT_PROC_H_  ----- 


