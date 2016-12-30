/**
 **/

#ifndef  _ORS_PROCESS_NET_H_ 
#define  _ORS_PROCESS_NET_H_

#include <boost/serialization/singleton.hpp>
#include "src/comm_event/comm_event.h"
#include "src/comm_event/comm_event_interface.h"
#include "src/comm_event/comm_event_factory.h"
#include "src/ors/processor/ors_processor.h"

namespace poseidon
{
namespace ors
{

struct SessData
{
        uint64_t time_req;
        uint64_t time_query_start;
        uint64_t time_query_end;
        uint64_t time_rsp;
        AlgoRequest req;
        AlgoResponse rsp;
};

class ProcessNet:public dc::common::comm_event::CommBase, public boost::serialization::singleton<ProcessNet> 
{

public:
    virtual int Init(const char* algo_conf_file);
    virtual int handle_read(const char * buf, const int len, struct sockaddr_in & client_addr);

private:
    OrsProcessor m_ors_processor;
};

} // ors
} // poseidon

#endif // #ifndef _ORS_PROCESS_NET_H_

