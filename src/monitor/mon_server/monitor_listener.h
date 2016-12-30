/**
 **/

#ifndef  _MONITOR_LISTENER_H_ 
#define  _MONITOR_LISTENER_H_

#include <boost/serialization/singleton.hpp>
#include "protocol/src/poseidon_proto.h"
#include "comm_event_tcp_listener.h"
#include "comm_event_tcp.h"
#include "comm_event_factory.h"

namespace poseidon
{
namespace monitor
{
class MonitorListener:public dc::common::comm_event::CommTcpListener,public boost::serialization::singleton<MonitorListener>
{
public:


    /**
     * @brief               连接到来时调用
     **/
    virtual int on_accept(evutil_socket_t fd, struct sockaddr *address, int socklen);

    /**
     * @brief               
     **/
    virtual int on_accept_error();

private:
};

}//monitor
}//poseidon

#endif   // ----- #ifndef _MONITOR_LISTENER_H_  ----- 



