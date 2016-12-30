/**
 **/

#include "process_sn.h"
#include "util/log.h"
#include "monitor_api.h"
#include "../main/config.h"
#include "ha.h"
#include "protocol/src/poseidon_proto.h"
#include "src/control/main/session_manager.h"
#include "util/func.h"
#include "../common/control_attr.h"

namespace poseidon{
namespace control{

/**
 * @brief               process req package
 **/
int ProcessSn::handle_read(const char * buf, const int len, struct sockaddr_in & client_addr)
{
    int rt=0;
    do{
        MON_ADD(ATTR_SN_RSP, 1);
        sn::SNResponse rsp;
        if( !rsp.ParseFromArray(buf, len) )
        {
            MON_ADD(ATTR_SN_UNPACK_ERR, 1);
            LOG_ERROR("sn res ParseFromArray error,buf len=",len);
            rt=-1;
            break;
        }
        LOG_DEBUG("snrsp[%s]\n", rsp.DebugString().c_str());
        rt=proc_result(rsp);
//        int sid = req.session_id();
        if(rt != 0)
        {
            LOG_ERROR("proc_result return error\n");
            break;
        }
    }while(0);
    return rt;
}


/**
 * @brief       send req to qp server
 * @param req   [IN], req
 * @return      success return 0, or return other error code
 **/
int ProcessSn::query(sn::SNRequest & req)
{
    int rt=0;
    std::string sendstr;
    do{
        LOG_DEBUG("ProcessSn::query req[%s]\n", req.DebugString().c_str());
        if(!req.SerializeToString(&sendstr))
        {
            LOG_ERROR("req.SerializeToString return false");
            rt=-1;
            break;
        }
        struct sockaddr_in addr;
        rt=get_sn_addr(&addr);
        if(rt != 0)
        {
            MON_ADD(ATTR_SN_GET_ADDR_ERR, 1);
            LOG_ERROR("get_sn_addr error");
            rt=-2;
            break;
        }
//        util::Func::get_time_ms(pSess->data().time_snreq);
        send_pkg(sendstr.c_str(), sendstr.length(), addr );
        MON_ADD(ATTR_SN_REQ, 1);
    }while(0);
    return rt;
}


/**
 * @brief       when recv a rsp called by handle_read
 * @param rsp   [IN], rsp
 * @return      success return 0, or return other code
 **/
int ProcessSn::proc_result(sn::SNResponse & rsp)
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
        util::Func::get_time_ms(pSess->data().time_snrsp);
//      MON_ADD("sn.latency", (int32_t)(pSess->data().time_snrsp-pSess->data().time_snreq) );
        pSess->data().snrsp=rsp;
        pSess->status(STAT_GET_SN);
        pSess->dispatch();
    }while(0);
    return rt;
}



/**
 * @brief       get qp addr
 * @param addr  [OUT], return a addr of qp
 * @return      success return 0, or return other error
 **/
int ProcessSn::get_sn_addr(struct sockaddr_in * addr)
{
    int rt=0;
    do{
        if(Config::get_mutable_instance().ha_on())
        {
            rt=HA_GET_ADDR("sn", (*addr) );
            if(rt != 0)
            {
                LOG_ERROR("HA_GET_ADDR error");
                break;
            }
            LOG_DEBUG("sn addr[%s]", util::Func::to_str(*addr).c_str());
        }else
        {
            MakeAddr((*addr), "10.32.50.245", 25800);
        }
    }while(0);
    return rt;
}

}//control
}//poseidon
