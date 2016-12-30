/**
 **/

#ifndef  _PROCESS_FEEDBACK_H_ 
#define  _PROCESS_FEEDBACK_H_

#include "boost/serialization/singleton.hpp"
#include "comm_event.h"
#include "comm_event_interface.h"
#include "comm_event_factory.h"
#include "protocol/src/poseidon_proto.h"

namespace poseidon
{
namespace control
{

class ProcessFeedback:public dc::common::comm_event::CommBase, public boost::serialization::singleton<ProcessFeedback> 
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
    int query(feedback::FeedbackRequest & req);


    /**
     * @brief       when recv a rsp called by handle_read
     * @param rsp   [IN], rsp
     * @return      success return 0, or return other code
     **/
    int proc_result(feedback::FeedbackResponse & rsp);



    /**
     * @brief       get qp addr
     * @param addr  [OUT], return a addr of qp
     * @return      success return 0, or return other error
     **/
    int get_feedback_addr(struct sockaddr_in * addr);

};

}//control
}//poseidon

#endif   // ----- #ifndef _PROCESS_FEEDBACK_H_  ----- 


