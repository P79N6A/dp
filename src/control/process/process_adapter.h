/**
 **/

#ifndef  _PROCESS_ADAPTER_H_ 
#define  _PROCESS_ADAPTER_H_

#include <boost/serialization/singleton.hpp>
#include "comm_event.h"
#include "comm_event_interface.h"
#include "comm_event_factory.h"
#include "protocol/src/poseidon_proto.h"
#include "../main/session_manager.h"

namespace poseidon{
namespace control{

class ProcessAdapter:public dc::common::comm_event::CommBase, public boost::serialization::singleton<ProcessAdapter> 
{
public:
    /**
     * @brief               process req package
     **/
    virtual int handle_read(const char * buf, const int len, struct sockaddr_in & client_addr);


    
    /**
     * @brief               response to client
     **/
    int response_client(Session * sess);

};

}//control
}//poseidon

#endif   // ----- #ifndef _PROCESS_ADAPTER_H_  ----- 

