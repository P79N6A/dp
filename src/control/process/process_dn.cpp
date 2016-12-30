/**
 **/
#include "process_dn.h"
#include "util/log.h"
#include "util/func.h"
#include "monitor_api.h"
#include "../main/config.h"
#include "ha.h"
#include "protocol/src/poseidon_proto.h"
#include "src/control/main/session_manager.h"
#include "../common/control_attr.h"
#include "util/hexdump.h"
namespace poseidon{
namespace control{

/**
 * @brief               process req package
 **/
int ProcessDn::handle_read(const char * buf, const int len, struct sockaddr_in & client_addr)
{
    int rt=0;
    do{
        MON_ADD(ATTR_DN_RSP, 1);
        dn::DNResponse rsp;
        if( !rsp.ParseFromArray(buf, len) )
        {
            LOG_ERROR("dn rsp ParseFromArray error,recv len=%d",len);
            //HEXDUMP(buf, len, LOG_ERROR);
            MON_ADD(ATTR_DN_RSP_PARSE_ERROR, 1);
            rt=-1;
            break;
        }
        LOG_DEBUG("dnrsp[%s]\n", rsp.DebugString().c_str() );
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
int ProcessDn::query(dn::DNRequest & req)
{
    int rt=0;
    std::string sendbuf;
    do{
        LOG_DEBUG("ProcessDn::query req[%s]\n", req.DebugString().c_str());
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
            MON_ADD(ATTR_DN_GET_ADDR, 1);
            LOG_ERROR("get_addr error");
            rt=-2;
            break;
        }
        send_pkg(sendbuf.c_str(), sendbuf.length(), addr );
        MON_ADD(ATTR_DN_REQ, 1);
    }while(0);
    return rt;
}


/**
 * @brief  获取结果
 **/
int ProcessDn::proc_result(dn::DNResponse & rsp)
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
        
        util::Func::get_time_ms(pSess->data().time_dnrsp);
//        MON_ADD("dn.latency", (int32_t)(pSess->data().time_dnrsp-pSess->data().time_dnreq) );
        pSess->data().dnrsp=rsp;
        pSess->status(STAT_GET_DN);
        pSess->dispatch();

    }while(0);
    return rt;
}

/**
 * @brief       get addr
 * @param addr  [OUT], return a addr of dn 
 * @return      success return 0, or return other error
 **/
int ProcessDn::get_addr(struct sockaddr_in * addr)
{
    int rt=0;
    do{
        if(Config::get_mutable_instance().ha_on())
        {
            rt=HA_GET_ADDR("dn", (*addr) );
            if(rt != 0)
            {
                LOG_ERROR("HA_GET_ADDR error");
                break;
            }
            LOG_DEBUG("dn addr[%s]", util::Func::to_str(*addr).c_str());
        }else
        {
            MakeAddr((*addr), "10.32.50.214", 26000  );
        }
    }while(0);
    return rt;
};

}//control
}//poseidon


