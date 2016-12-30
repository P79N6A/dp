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

namespace poseidon {
namespace control {

int Scheduler::process_fb_get(Session * sess) {
    int rt = 0;
    do {
        sess->data().prog |= PROG_FB_RSP;

        /* Add logic*/
        feedback::FeedbackResponse & fbrsp = sess->data().fbrsp;
        sn::SNResponse & snrsp = sess->data().snrsp;
        std::map < uint32_t, uint32_t > feedback_map;
        std::set < uint32_t > &setfilter = sess->data().setfilter;

        int fb_size = fbrsp.feedbackinfo_size();
        for (int i = 0; i < fb_size; i++) {
            common::FeedbackInfo * fbinfo = fbrsp.mutable_feedbackinfo(i);
            feedback_map[fbinfo->adgroup_id()] = i;
        }
        if (snrsp.ads_size() == 0) {
            sess->status(STAT_FAIL);
            process_fail(sess);
            LOG_ERROR("sn resp has no ads");
            rt = -1;
            break;
        }
        const sn::Ads & ads = snrsp.ads(0);
        int ad_size = ads.ad_size();
        for (int i = 0; i < ad_size; i++) {
            const common::Ad & ad = ads.ad(i);
            uint32_t adgroup_id = ad.adgroup_id();
            if (feedback_map.count(adgroup_id) == 0) { //没有反馈数据，放过
                continue;
            }
            std::map<uint32_t, uint32_t>::iterator iter = feedback_map.find(
                    adgroup_id);
            if (iter == feedback_map.end())
                continue;
            const common::FeedbackInfo & fbinfo = fbrsp.feedbackinfo(
                    iter->second);
            //pdb直投不做任何过滤
            if (fbinfo.has_pdb_feedback()) {

                continue;
            } else {
                if (ad.freq_impression() > 0
                        && (fbinfo.adgroup_user_day_freq()
                                > ad.freq_impression())) {
                    setfilter.insert(adgroup_id);
                    continue;
                }

                int32_t advertiser_balance = -1;
                if (ad.has_advertiser_balance()) {
                    advertiser_balance = ad.advertiser_balance();
                }
                LOG_DEBUG("advertiser_balance %d", advertiser_balance);
                if (fbinfo.has_advertiser_last_day_cost()) {
                    advertiser_balance = advertiser_balance
                            - fbinfo.advertiser_last_day_cost();
                    LOG_DEBUG("advertiser_last_day_cost %d",
                            fbinfo.advertiser_last_day_cost());
                }
                LOG_DEBUG("advertiser_balance %d", advertiser_balance);
                if (fbinfo.advertiser_day_cost() >= ad.advertiser_budget()) {
                    LOG_DEBUG("insert filter %d", adgroup_id);
                    setfilter.insert(adgroup_id);
                    continue;
                }
                if (fbinfo.campaign_day_cost() >= ad.campaign_daily_budget()) {
                    LOG_DEBUG("insert filter %d", adgroup_id);
                    setfilter.insert(adgroup_id);
                    continue;
                }
                if (advertiser_balance != -1
                        && fbinfo.advertiser_day_cost() >= advertiser_balance) {
                    LOG_DEBUG("insert filter %d", adgroup_id);
                    setfilter.insert(adgroup_id);
                    continue;
                }
            }
        }

        rt = build_dn_req(sess);
        if (rt != 0) {
            sess->status(STAT_FAIL);
            process_fail(sess);
            //LOG_ERROR("build_dn_req error, rt[%d]\n", rt);
            rt = -1;
            break;
        }
        util::Func::get_time_ms(sess->data().time_dnreq);
        rt = ProcessDn::get_mutable_instance().query(sess->data().dnreq);
        if (rt != 0) {
            LOG_ERROR("ProcessDn query return error, rt[%d]\n", rt);
            rt = -1;
            break;
        }
        sess->data().prog |= PROG_DN_REQ;
    } while (0);
    return rt;
}

int Scheduler::build_fb_req(Session *sess) {
    int rt = 0;
    do {
        rtb::BidRequest & bidreq = sess->data().bidreq;
        sn::SNResponse & snrsp = sess->data().snrsp;
        feedback::FeedbackRequest & fbreq = sess->data().fbreq;

        int sid = sess->sid();

        fbreq.set_trace_id(sess->data().trace_id);

        fbreq.set_session_id(util::Func::to_str(sid));
        
        fbreq.set_source(sess->data().traffic_source);

        const rtb::BidRequest::Ext& bidreqext = bidreq.ext();
        SET_VALUE(fbreq, bidreqext, dspid, dsp_id);

        const rtb::User & biduser = bidreq.user();
        if (biduser.has_ext()) {
            const rtb::User::Ext & user_ext = biduser.ext();
            SET_VALUE(fbreq, user_ext, aid, aid);
            SET_VALUE(fbreq, user_ext, acookie, acookie);
        }
        if (bidreq.has_device() && bidreq.device().has_id()) {
            fbreq.set_dev_id(bidreq.device().id());
        }

        google::protobuf::RepeatedPtrField<poseidon::common::Ad> * ad =
                fbreq.mutable_ad();

        /*TODOFIX: 暂时只考虑一个广告位的情况, 待处理多个广告位*/
        const poseidon::sn::Ads & ads = snrsp.ads(0);
        ad->CopyFrom(ads.ad());

    } while (0);
    return rt;
}

}
}
