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
 * @brief           called when get Qp Rsp after
 **/
int Scheduler::process_qp_get(Session * sess) {
    int rt = 0;
    do {
        sess->data().prog |= PROG_QP_RSP;
        rt = build_sn_req(sess);
        if (rt != 0) {
            LOG_ERROR("build_sn_req error, rt[%d]\n", rt);
            rt = -1;
            break;
        }
        util::Func::get_time_ms(sess->data().time_snreq);
        rt = ProcessSn::get_mutable_instance().query(sess->data().snreq);
        if (rt != 0) {
            LOG_ERROR("ProcessSn query return error, rt[%d]\n", rt);
            rt = -1;
            break;
        }
        sess->data().prog |= PROG_SN_REQ;
    } while (0);
    return rt;
}

int Scheduler::build_qp_req(Session * sess) {
    int rt = 0;
    do {
        qp::QPRequest & qpreq = sess->data().qpreq;
        rtb::BidRequest & bidreq = sess->data().bidreq;

        // session id
        int sid = sess->sid();

        qpreq.set_trace_id(sess->data().trace_id);
        qpreq.set_session_id(util::Func::to_str(sid));

        // dsp id
        if (bidreq.has_ext()) {
            if (bidreq.ext().has_dsp_id()) {
                qpreq.set_dsp_id(bidreq.ext().dsp_id());
            }
        }
        google::protobuf::RepeatedPtrField<poseidon::rtb::Impression>* impression =
                qpreq.mutable_impressions();
        impression->CopyFrom(bidreq.impressions());
        // site
        if (bidreq.has_site()) {
            rtb::Site* site = qpreq.mutable_site();
            site->CopyFrom(bidreq.site());
        }
        // user
        if (bidreq.has_user()) {
            rtb::User* user = qpreq.mutable_user();
            user->CopyFrom(bidreq.user());
        }
        // device
        if (bidreq.has_device()) {
            rtb::Device* device = qpreq.mutable_device();
            device->CopyFrom(bidreq.device());
        }
        // app
        if (bidreq.has_app()) {
            rtb::App* app = qpreq.mutable_app();
            app->CopyFrom(bidreq.app());
        }

    } while (0);
    return rt;

}

}
}
