/**
 **/

#ifndef  _LOG_SERVER_H_ 
#define  _LOG_SERVER_H_


#include <boost/serialization/singleton.hpp>
#include "comm_event.h"
#include "comm_event_interface.h"
#include "comm_event_factory.h"
#include "protocol/src/poseidon_proto.h"

namespace poseidon{
namespace log{


class LogServer:public dc::common::comm_event::CommBase, public boost::serialization::singleton<LogServer> 
{
public:

    /**
     * @brief               process req package
     **/
    virtual int handle_read(const char * buf, const int len, struct sockaddr_in & client_addr);

};

}//log
}//poseidon

#endif   // ----- #ifndef _LOG_SERVER_H_  ----- 

