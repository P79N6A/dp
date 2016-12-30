/**
 **/
#include <unistd.h>

#include "src/ors/bin/process_net.h"
#include "util/log.h"
#include "util/func.h"
#include "src/monitor/api/monitor_api.h"
#include "protocol/src/poseidon_proto.h"
#include "src/ors/bin/config.h"

namespace poseidon {
namespace ors {

int ProcessNet::Init(const char* algo_conf_file)
{
    srand(time(NULL));

    bool stat_on = false;
    if (Config::get_mutable_instance().stat_on() == 1 &&
        Config::get_mutable_instance().process_idx() == 0)
    {
        stat_on = true;
    }

    LOG_INFO("process_idx=%d, stat_on=%d", 
            Config::get_mutable_instance().process_idx(), 
            stat_on);
    int ret = m_ors_processor.Init(algo_conf_file, stat_on);
    if (ret != 0) 
    {
        LOG_ERROR("OrsProcessor Init Failed!ret = %d", ret);
        return ret;
    }

    LOG_INFO("ProcessNet Init OK!");
    return 0;
}

int ProcessNet::handle_read(const char * buf, const int len, struct sockaddr_in & client_addr)
{
    int rt=0;
    SessData * sess=NULL;
    do{
        sess = new(std::nothrow) SessData();
        if(sess == NULL)
        {
            break;
        }

        util::Func::get_time_ms(sess->time_req);

        AlgoRequest& req = sess->req;
        AlgoResponse& rsp = sess->rsp;

        MON_ADD(ATTR_SVR_ORS_REQ_COUNT, 1);
        if(!req.ParseFromArray(buf, len))
        {
            MON_ADD(ATTR_SVR_ORS_PARSE_REQ_PROTO_ERROR_COUNT, 1);
            LOG_ERROR("req ParseFromArray error");
            rt = -1;
            break;
        }
        LOG_DEBUG("recv ors req[%s]\n", req.DebugString().c_str());

        MON_ADD(ATTR_SVR_ORS_AVG_AD_NUM, req.algo_ads_size());
        int ret = m_ors_processor.Process(req, &rsp);
        if (ret != 0)
        {
            MON_ADD(ATTR_SVR_ORS_INNER_PROCESS_ERROR_COUNT, 1);
            rt = -2;
            break;
        } 
        
        if (rsp.algoed_ads_size() == 0) {
            rsp.set_error_code(common::ERROR_NO_RESULT);
            MON_ADD(ATTR_SVR_ORS_NOT_RSP_TOPN_COUNT, 1);
        } else {
            rsp.set_error_code(common::ERROR_NONE);
            MON_ADD(ATTR_SVR_ORS_HAS_RSP_TOPN_COUNT, 1);
        }

        sess->rsp.set_session_id(req.session_id());
        if (req.has_trace_id())
        {
            sess->rsp.set_trace_id(req.trace_id());
        }
        char hostname[256];
        memset(hostname, 0x00, 256); 
        gethostname(hostname, 256);
        rsp.set_hostname(hostname);

        std::string rspstr = "";
        if(!rsp.SerializeToString(&rspstr))
        {
            LOG_ERROR("rsp.SerializeToString error");
            rt=-3;
            break;
        }
        LOG_DEBUG("orsrsp[%s], rsp.ByteSize[%u], rspstr.length[%u]", rsp.DebugString().c_str(), rsp.ByteSize(), rspstr.length());
        if (Config::get_mutable_instance().send_on()) {
            send_pkg(rspstr.c_str(), rspstr.length(), client_addr);
            MON_ADD(ATTR_SVR_ORS_RSP_COUNT, 1);
        }
        util::Func::get_time_ms(sess->time_rsp);
        MON_ADD(ATTR_SVR_ORS_AVG_PROCESS_TIME, (int)(sess->time_rsp-sess->time_req));

    }while(0);
    if(sess != NULL)
    {
        delete sess;
    }
    return rt;
}

}
}


