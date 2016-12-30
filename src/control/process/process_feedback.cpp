/**
 **/

#include "process_feedback.h"
#include "util/log.h"
#include "monitor_api.h"
#include "util/func.h"
#include "util/hexdump.h"
#include "../main/config.h"
#include "ha.h"
#include "protocol/src/poseidon_proto.h"
#include "src/control/main/session_manager.h"
#include "../common/control_attr.h"
#include <string>

namespace poseidon{
namespace control{

/**
 * @brief               process req package
 **/
int ProcessFeedback::handle_read(const char * buf, const int len, struct sockaddr_in & client_addr)
{
    int rt=0;
    do{
        MON_ADD(ATTR_FB_RSP, 1);
        feedback::FeedbackResponse rsp;
        if( !rsp.ParseFromArray(buf, len) )
        {
            MON_ADD(ATTR_FB_RSP_PARSE_ERROR, 1);
            LOG_ERROR("fb res ParseFromArray error,buf len=",len);
            rt=-1;
            break;
        }
        LOG_DEBUG("fbrsp[%s]\n", rsp.DebugString().c_str());
        rt=proc_result(rsp);
        if(rt != 0)
        {
            LOG_ERROR("proc_result return error\n");
            break;
        }


    }while(0);
    return rt;
}

/**
 * @brief  发起查询
 **/
int ProcessFeedback::query(feedback::FeedbackRequest & req)
{
    int rt=0;
    std::string sendbuf;
    do{
        LOG_DEBUG("ProcessFeedback::query req[%s]\n", req.DebugString().c_str());
        if(!req.SerializeToString(&sendbuf))
        {
            LOG_ERROR("req.SerializeToString return false");
            rt=-1;
            break;
        }
        struct sockaddr_in addr;
        rt=get_feedback_addr(&addr);
        if(rt != 0)
        {
            MON_ADD(ATTR_FB_GET_ADDR_ERR, 1);
            LOG_ERROR("get_feedback_addr error");
            rt=-2;
            break;
        }
//        int iLen=sendbuf.length();
//        HEXDUMP(sendbuf.c_str(), iLen, LOG_DEBUG);
        send_pkg(sendbuf.c_str(), sendbuf.length(), addr );
        MON_ADD(ATTR_FB_REQ, 1);

    }while(0);
    return rt;
}


/**
 * @brief  获取结果
 **/
int ProcessFeedback::proc_result(feedback::FeedbackResponse & rsp)
{
    int rt=0;
    do{
        int sid=util::Func::to_int(rsp.session_id());
        Session * pSess=SessionManager::get_mutable_instance().get_session(sid);
        if(pSess == NULL)
        {
            MON_ADD(ATTR_GET_SESS_ERR, 1);
            LOG_ERROR("SessionManager get_session error");
            rt=-1;
            break;
        }
        
        if(rsp.has_trace_id())
        {
          const rtb::BidRequest & bidreq=pSess->data().bidreq;
          if(bidreq.has_trace_id())
          {
            if(rsp.trace_id().compare(bidreq.trace_id())!=0)
            {
              rt=-2;
              LOG_ERROR("SessionManager get_session error,trace_id no eq");
              break;
            }
          }
        }
        
        util::Func::get_time_ms(pSess->data().time_fbrsp);
//        MON_ADD("fb.latency", (int32_t)(pSess->data().time_fbrsp-pSess->data().time_fbreq) );
        pSess->data().fbrsp=rsp;
        pSess->status(STAT_GET_FB);
        pSess->dispatch();

    }while(0);
    return rt;
}

/**
 * @brief       get addr
 * @param addr  [OUT], return a addr 
 * @return      success return 0, or return other error
 **/
int ProcessFeedback::get_feedback_addr(struct sockaddr_in * addr)
{
    int rt=0;
    do{
        if(Config::get_mutable_instance().ha_on())
        {
            rt=HA_GET_ADDR("fb", (*addr) );
            if(rt != 0)
            {
                LOG_ERROR("HA_GET_ADDR error");
                break;
            }
            LOG_DEBUG("fb addr[%s]", util::Func::to_str(*addr).c_str());
        }else
        {
            MakeAddr((*addr), "10.32.50.180", 25900);
        }
    }while(0);
    return rt;
};

}//control
}//poseidon

