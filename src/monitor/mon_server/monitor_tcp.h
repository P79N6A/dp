/**
 **/


#ifndef  _MONITOR_TCP_H_ 
#define  _MONITOR_TCP_H_

#include "comm_event_tcp.h"
#include "comm_event_factory.h"
#include "protocol/src/poseidon_proto.h"

namespace poseidon
{
namespace monitor
{
class MonitorConn:public dc::common::comm_event::CommTcpBase
{
public:

    /** 
     * @brief
     * @return
     **/
    virtual int handle_pkg(const char * buf, const int len);

    /** 
     * @brief
     * @param
     * @return
     **/
    virtual int on_error();
    
    /** 
     * @brief
     * @param
     * @return
     **/
    virtual int on_close();


    /**
     * @brief               上报入库
     **/
    int report2db(ReportInfoReq & req);

};
}//monitor
}//poseidon

#endif   // ----- #ifndef _MONITOR_TCP_H_  ----- 

