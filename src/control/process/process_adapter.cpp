/**
 **/
#include "process_adapter.h"
#include "util/log.h"
#include "monitor_api.h"
#include "util/func.h"
#include "protocol/src/poseidon_proto.h"
#include "src/control/main/session_manager.h"
#include "../common/control_attr.h"

namespace poseidon{
namespace control{

int ProcessAdapter::handle_read(const char * buf, const int len, struct sockaddr_in & client_addr)
{
    int rt=0;
    int sid=-1;
    do{
        MON_ADD(ATTR_CONTROL_REQ, 1);
        rtb::BidRequest req ;
        if( !req.ParseFromArray(buf, len) )
        {
            LOG_ERROR("req ParseFromArray error,buf len=%d",len);
            MON_ADD(ATTR_ADAPTER_REQ_PARSE_ERROR, 1);
            rt = -1;
            break;
        }
        LOG_DEBUG("recv adapter package[%s]\n", req.DebugString().c_str());

        sid = SessionManager::get_mutable_instance().alloc_session();
        if(sid < 0)
        {
            MON_ADD(ATTR_ALLOC_SESSION_ERR, 1);
            LOG_ERROR("alloc_session error");
            rt=-3;
            break;
        }
        Session* pSess=SessionManager::get_mutable_instance().get_session(sid);
        if(pSess == NULL)
        {
            MON_ADD(ATTR_GET_SESSION_ERR, 1);
            LOG_ERROR("get_session return error, sid[%d]", sid);
            rt=-3;
            break;
        }
        util::Func::get_time_ms(pSess->data().time_bidreq);
        pSess->data().reqpack.assign(buf, len);
        pSess->data().bidreq=req;  //call CopyFrom
        pSess->status(STAT_GET_ADAPTER);
        memcpy(&(pSess->data().client_addr), &client_addr, sizeof(struct sockaddr_in) );
        pSess->dispatch();

    }while(0);
    if(rt != 0)
    {
        if(sid >= 0)
        {
            SessionManager::get_mutable_instance().free_session(sid);
        }
    }
    return rt;
}

/**
 * @brief               response to client
 **/
int ProcessAdapter::response_client(Session * sess)
{
    int rt=0;
    do{
        std::string send_buf;
        rtb::BidResponse & bidrsp=sess->data().bidrsp;
        if(!bidrsp.SerializeToString(&send_buf))
        {
            LOG_ERROR("SerializeToString error");
            rt=-2;
            break;
        }
        LOG_DEBUG("bidrsp[%s]\n", bidrsp.DebugString().c_str());

        util::Func::get_time_ms(sess->data().time_bidrsp);
        send_pkg(send_buf.c_str(), send_buf.length(), sess->data().client_addr);
        MON_ADD(ATTR_CONTROL_RSP, 1);

//        MON_ADD("adapter.latency", (int32_t)(sess->data().time_bidrsp-sess->data().time_bidreq) );
    }while(0);
//    SessionManager::get_mutable_instance().free_session(sid);
    return rt;
}


}
}


