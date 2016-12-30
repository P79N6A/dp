#include "scheduler.h"
#include "config.h"
#include "util/log.h"
#include "util/func.h"
#include "util/monitor.h"
#include "../common/control_attr.h"
#include "monitor_api.h"
#include "util/pvlog.h"
#include "src/control/process/control_process.h"
#include <sstream>
#include "pvlog_trans.h"
#include "format_filter.h"
#include "json/json.h"

namespace poseidon {
namespace control {

/**
 * @brief            called when get Dn Rsp after 
 **/
int Scheduler::process_dn_get(Session * sess) {
    int rt = 0;
    do {
        sess->data().prog |= PROG_DN_RSP;

        if (sess->data().dnrsp.creative_size() == 0) {
            sess->status(STAT_FAIL);
            process_fail(sess);
            break;
        }

        rt = build_ors_req(sess);
        if (rt != 0) {
            //由于返回-2为正常现象，所以只有-1时打印错误日志,build_ors_req修改时需要注意
            if (-1 == rt)
                LOG_ERROR("build_ors_req error, rt[%d]\n", rt);
            rt = -1;
            break;
        }
        util::Func::get_time_ms(sess->data().time_orsreq);
        rt = ProcessOrs::get_mutable_instance().query(sess->data().orsreq);
        if (rt != 0) {
            LOG_ERROR("ors query return error, rt[%d]\n", rt);
            rt = -1;
            break;
        }
        sess->data().prog |= PROG_ORS_REQ;
    } while (0);
    return rt;
}

int Scheduler::build_dn_req(Session *sess) {
    int rt = 0;
    do {
        dn::DNRequest & dnreq = sess->data().dnreq;
        rtb::BidRequest & bidreq = sess->data().bidreq;
        dnreq.set_trace_id(sess->data().trace_id);

        {
            sn::SNResponse & snrsp = sess->data().snrsp;
            std::set < uint32_t > &setfilter = sess->data().setfilter;

            int sid = sess->sid();
            dnreq.set_session_id(util::Func::to_str(sid));

            int ads_size = snrsp.ads_size();
            int idx = 0;
            for (idx = 0; idx < ads_size; idx++) {
                const sn::Ads & ads = snrsp.ads(idx);
                int ad_size = ads.ad_size();
                for (int i = 0; i < ad_size; i++) {
                    const common::Ad & ad = ads.ad(i);
                    if (setfilter.count(ad.adgroup_id()) == 0) {
                        dnreq.add_creative_ids(ad.creative_id());
                    }
                }
            }

        }
        if (dnreq.creative_ids_size() == 0) {
            MON_ADD(ATTR_DN_REQ_CID_ZERO, 1);
            //请求创意数为0是正常现象，中断流程，不打印错误日志
            //LOG_ERROR("Dn 请求创意ID数为0");
            rt = -1;
            break;
        }
    } while (0);
    return rt;
}

}
}
