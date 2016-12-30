/**
 **/
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
#include "yunos_game_map.h"

#define SET_VALUE(dst, src, dstname, srcname) \
    if (src.has_##srcname()) \
    { \
        dst.set_##dstname(src.srcname()); \
    }
#define SET_VALUE_PTR(dst, src, dstname, srcname) \
    if (src.has_##srcname()) \
    { \
        dst->set_##dstname(src.srcname()); \
    }

#define TODOFIX(A) do{}while(0)

namespace poseidon {
namespace control {
/*status:Adapter-->Qp-->Sn-->Fc-->Dn-->Ors-->Adapter*/
/*new status(2016/7/12):Adapter-->Qp-->Sn-->Fb-->Dn-->Ors-->Adapter*/
int Scheduler::dispatch(Session * sess) {
    int rt = 0;
    do {
        /**/
        switch (sess->status()) {
        case STAT_GET_ADAPTER:
            //process get adapter get
            rt = process_adapter_get(sess);
            break;
        case STAT_GET_QP:
            //process get Qp get
            rt = process_qp_get(sess);
            break;
        case STAT_GET_SN:
            //process get Qp get
            rt = process_sn_get(sess);
            break;
        case STAT_GET_FB:
            rt = process_fb_get(sess);
            break;
        case STAT_GET_DN:
            rt = process_dn_get(sess);
            break;
        case STAT_GET_ORS:
            rt = process_ors_get(sess);
            break;
        case STAT_FAIL:
            rt = process_fail(sess);
            break;
        case STAT_TIME_OUT:
            rt = process_timeout(sess);
            break;
        default:
            LOG_WARN("unkowned status[%d]\n", sess->status());
            break;
        }
    } while (0);
    if (rt != 0) {
//        ProcessAdapter::get_mutable_instance().response_client(sess->sid());
    }
    return rt;
}

/**
 * @brief           called on session timeout
 **/
int Scheduler::process_fail(Session * sess) {
    LOG_DEBUG("process_fail,trace_id : %s", sess->data().trace_id.c_str());
    int rt = 0;
    do {
        //兜底广告
        if (enable_backup_ad(sess)) {
            ors::AlgoResponse & orsrsp = sess->data().orsrsp;
            orsrsp.set_error_code(common::ERROR_BACKUP_AD);
            orsrsp.set_trace_id(sess->data().trace_id);
            process_ors_get(sess);
            break;
        }

        //step 1:返回给Adapter
        rt = build_error_adapter_rsp(sess);
        if (rt != 0) {
            LOG_ERROR("build error adapter rsp error, rt[%d]\n", rt);
            rt = -1;
            break;
        }
        rt = ProcessAdapter::get_mutable_instance().response_client(sess);
        if (rt != 0) {
            LOG_ERROR("Adapter response_client error, rt[%d]\n", rt);
            rt = -2;
            break;
        }
        MON_ADD(ATTR_FAIL_RSP, 1);

        //step 2:pv_log
        write_log(sess);

    } while (0);
    sess->free();
    return rt;
}

/**
 * @brief           called on session timeout
 **/
int Scheduler::process_timeout(Session * sess) {
    //TODO: TIMEOUT something
    int & prog = sess->data().prog;
    if ((prog & PROG_QP_REQ) && !(prog & PROG_QP_RSP)) {
        MON_ADD(ATTR_QP_TIMEOUT, 1);
    } else if ((prog & PROG_SN_REQ) && !(prog & PROG_SN_RSP)) {
        MON_ADD(ATTR_SN_TIMEOUT, 1);
    } else if ((prog & PROG_FB_REQ) && !(prog & PROG_FB_RSP)) {
        MON_ADD(ATTR_FB_TIMEOUT, 1);
    } else if ((prog & PROG_DN_REQ) && !(prog & PROG_DN_RSP)) {
        MON_ADD(ATTR_DN_TIMEOUT, 1);
    } else if ((prog & PROG_ORS_REQ) && !(prog & PROG_ORS_RSP)) {
        MON_ADD(ATTR_ORS_TIMEOUT, 1);
    }
    MON_ADD(ATTR_CONTROLER_TIMEOUT, 1);

    return process_fail(sess);
}

void Scheduler::status_bid_latency(uint64_t bid_latency) {
    if (bid_latency < 10) {
        MON_ADD(ATTR_BID_LATENCY_LESS_10, 1);
    } else if (bid_latency < 30) {
        MON_ADD(ATTR_BID_LATENCY_LESS_30, 1);
    } else if (bid_latency < 50) {
        MON_ADD(ATTR_BID_LATENCY_LESS_50, 1);
    } else {
        MON_ADD(ATTR_BID_LATENCY_GT_50, 1);
    }
}

void Scheduler::status_qp_latency(uint64_t latency) {
    if (latency < 5) {
        MON_ADD(ATTR_QP_LATENCY_LESS_5, 1);
    } else if (latency < 10) {
        MON_ADD(ATTR_QP_LATENCY_LESS_10, 1);
    } else if (latency < 20) {
        MON_ADD(ATTR_QP_LATENCY_LESS_20, 1);
    } else if (latency < 50) {
        MON_ADD(ATTR_QP_LATENCY_LESS_50, 1);
    } else {
        MON_ADD(ATTR_QP_LATENCY_GT_50, 1);
    }
}
void Scheduler::status_sn_latency(uint64_t latency) {
    if (latency < 5) {
        MON_ADD(ATTR_SN_LATENCY_LESS_5, 1);
    } else if (latency < 10) {
        MON_ADD(ATTR_SN_LATENCY_LESS_10, 1);
    } else if (latency < 20) {
        MON_ADD(ATTR_SN_LATENCY_LESS_20, 1);
    } else if (latency < 50) {
        MON_ADD(ATTR_SN_LATENCY_LESS_50, 1);
    } else {
        MON_ADD(ATTR_SN_LATENCY_GT_50, 1);
    }
}
void Scheduler::status_fb_latency(uint64_t latency) {
    if (latency < 5) {
        MON_ADD(ATTR_FB_LATENCY_LESS_5, 1);
    } else if (latency < 10) {
        MON_ADD(ATTR_FB_LATENCY_LESS_10, 1);
    } else if (latency < 20) {
        MON_ADD(ATTR_FB_LATENCY_LESS_20, 1);
    } else if (latency < 50) {
        MON_ADD(ATTR_FB_LATENCY_LESS_50, 1);
    } else {
        MON_ADD(ATTR_FB_LATENCY_GT_50, 1);
    }
}
void Scheduler::status_dn_latency(uint64_t latency) {
    if (latency < 5) {
        MON_ADD(ATTR_DN_LATENCY_LESS_5, 1);
    } else if (latency < 10) {
        MON_ADD(ATTR_DN_LATENCY_LESS_10, 1);
    } else if (latency < 20) {
        MON_ADD(ATTR_DN_LATENCY_LESS_20, 1);
    } else if (latency < 50) {
        MON_ADD(ATTR_DN_LATENCY_LESS_50, 1);
    } else {
        MON_ADD(ATTR_DN_LATENCY_GT_50, 1);
    }
}
void Scheduler::status_ors_latency(uint64_t latency) {
    if (latency < 5) {
        MON_ADD(ATTR_ORS_LATENCY_LESS_5, 1);
    } else if (latency < 10) {
        MON_ADD(ATTR_ORS_LATENCY_LESS_10, 1);
    } else if (latency < 20) {
        MON_ADD(ATTR_ORS_LATENCY_LESS_20, 1);
    } else if (latency < 50) {
        MON_ADD(ATTR_ORS_LATENCY_LESS_50, 1);
    } else {
        MON_ADD(ATTR_ORS_LATENCY_GT_50, 1);
    }
}

bool Scheduler::enable_backup_ad(Session * sess) {
    LOG_DEBUG("enable_backup_ad,trace_id : %s", sess->data().trace_id.c_str());
    int prog = sess->data().prog;
    if ((prog & PROG_DN_RSP) == 0) {
        LOG_DEBUG("not after PROG_DN_RSP");
        return false;
    }
    if (sess->data().enabled_backup_ad) {
        LOG_DEBUG("enabled backup ad");
        return false;
    }
    rtb::BidRequest & bidreq = sess->data().bidreq;
    ors::AlgoRequest & orsreq = sess->data().orsreq;
    if (sess->data().traffic_source == rtb::TS_ALIGAME
            && bidreq.impressions_size() > 0 && bidreq.impressions(0).has_ext()
            && bidreq.impressions(0).ext().has_view_type()
            && bidreq.impressions(0).ext().view_type() == rtb::VT_WL_MIX_APP) {
        if (orsreq.algo_ads_size() > 0) {
            int backup_ad_idx = 0;
            ors::AlgoAd algo_ad = orsreq.algo_ads(backup_ad_idx);
            if (algo_ad.has_ad()) {
                LOG_DEBUG("enable backup ad,trace_id=%s",
                        sess->data().trace_id.c_str());
                return true;
            }
        }
    }
    return false;
}

} //control
} //poseidon

