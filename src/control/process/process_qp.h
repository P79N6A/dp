/**
 **/

#ifndef  _PROCESS_QP_H_ 
#define  _PROCESS_QP_H_

#include "boost/serialization/singleton.hpp"
#include "comm_event.h"
#include "comm_event_interface.h"
#include "comm_event_factory.h"
#include "protocol/src/poseidon_proto.h"

namespace poseidon
{
namespace control
{

class ProcessQp:public dc::common::comm_event::CommBase, public boost::serialization::singleton<ProcessQp> 
{
public:

    /**
     * @brief               process req package
     **/
    virtual int handle_read(const char * buf, const int len, struct sockaddr_in & client_addr);

    /**
     * @brief       send req to qp server
     * @param req   [IN], req
     * @return      success return 0, or return other error code
     **/
    int query(qp::QPRequest & req);


    /**
     * @brief       when recv a rsp called by handle_read
     * @param rsp   [IN], rsp
     * @return      success return 0, or return other code
     **/
    int proc_result(qp::QPResponse & rsp);



    /**
     * @brief       get qp addr
     * @param addr  [OUT], return a addr of qp
     * @return      success return 0, or return other error
     **/
    int get_qp_addr(struct sockaddr_in * addr);

};

}//control
}//poseidon

#endif   // ----- #ifndef _PROCESS_QP_H_  ----- 

