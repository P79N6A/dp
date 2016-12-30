/**
 **/
#include "process_ors.h"
#include "util/log.h"
#include "monitor_api.h"
#include "util/func.h"
#include "../main/config.h"
#include "ha.h"
#include "protocol/src/poseidon_proto.h"
#include "src/control/main/session_manager.h"
#include "../common/control_attr.h"


namespace poseidon{
namespace control{

/**
 * @brief               process req package
 **/
int ProcessOrs::handle_read(const char * buf, const int len, struct sockaddr_in & client_addr)
{
    int rt=0;
    do{
        MON_ADD(ATTR_ORS_RSP, 1);
        ors::AlgoResponse rsp;
        if( !rsp.ParseFromArray(buf, len) )
        {
          MON_ADD(ATTR_ORS_RSP_PARSE_ERROR, 1);
          LOG_ERROR("ors res ParseFromArray error,buf len=%d",len);
          rt=-1;
          break;
        }
        LOG_DEBUG("orsrsp[%s]", rsp.DebugString().c_str());
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
int ProcessOrs::query(ors::AlgoRequest & req)
{
    int rt=0;
    std::string sendbuf;
    do{
        LOG_DEBUG("ProcessOrs::query req[%s]\n", req.DebugString().c_str());
        if(!req.SerializeToString(&sendbuf))
        {
            LOG_ERROR("req.SerializeToString return false");
            rt=-1;
            break;
        }

        struct sockaddr_in addr;
        rt=get_addr(&addr);
        if(rt != 0)
        {
            MON_ADD(ATTR_ORS_GET_ADDR_ERR, 1);
            LOG_ERROR("get_addr error");
            rt=-2;
            break;
        }
        send_pkg(sendbuf.c_str(), sendbuf.length(), addr );
        MON_ADD(ATTR_ORS_REQ, 1);
    }while(0);
    return rt;
}


/**
 * @brief  获取结果
 **/
int ProcessOrs::proc_result(ors::AlgoResponse & rsp)
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
        
        util::Func::get_time_ms(pSess->data().time_orsrsp);
//        MON_ADD("ors.latency", (int32_t)(pSess->data().time_orsrsp-pSess->data().time_orsreq) );
        pSess->data().orsrsp=rsp;
        pSess->status(STAT_GET_ORS);
        pSess->dispatch();

    }while(0);
    return rt;
}

/**
 * @brief       get addr
 * @param addr  [OUT], return a addr
 * @return      success return 0, or return other error
 **/
int ProcessOrs::get_addr(struct sockaddr_in * addr)
{
    int rt=0;
    do{
        if(Config::get_mutable_instance().ha_on())
        {
            rt=HA_GET_ADDR("ors", (*addr) );
            if(rt != 0)
            {
                LOG_ERROR("HA_GET_ADDR error");
                break;
            }
            LOG_DEBUG("ors addr[%s]", util::Func::to_str(*addr).c_str());
        }else
        {
            MakeAddr((*addr), "10.32.50.244", 26100);
        }
    }while(0);
    return rt;
};

}//control
}//poseidon

