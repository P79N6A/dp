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

int Scheduler::process_sn_get(Session * sess) {
    int rt = 0;
    do {
        sess->data().prog |= PROG_SN_RSP;
        const poseidon::sn::SNResponse & snrsp = sess->data().snrsp;
//        std::map<uint64_t, common::Ad > & mapcad = sess->data().mapcad;
        int ad_size = snrsp.ads(0).ad_size();
        if (ad_size <= 0) {
            sess->status(STAT_FAIL);
            process_fail(sess);
            break;
        }

        { //get feedback info
            rt = build_fb_req(sess);
            if (rt != 0) {
                LOG_ERROR("build_fb_req error, rt[%d]\n", rt);
                rt = -1;
                break;
            }
            util::Func::get_time_ms(sess->data().time_fbreq);
            rt = ProcessFeedback::get_mutable_instance().query(
                    sess->data().fbreq);
            if (rt != 0) {
                LOG_ERROR("ProcessFeedback query return error, rt[%d]\n", rt);
                rt = -1;
                break;
            }
            sess->data().prog |= PROG_FB_REQ;
        }
    } while (0);
    return rt;
}

int Scheduler::build_sn_req(Session *sess) {
    int rt = 0;
    do {
        sn::SNRequest & snreq = sess->data().snreq;
        rtb::BidRequest & bidreq = sess->data().bidreq;
        qp::QPResponse & qprsp = sess->data().qprsp;

        int sid = sess->sid();

        if (qprsp.has_imei() && bidreq.has_device()) {
            rtb::Device* device = bidreq.mutable_device();
            device->set_id(qprsp.imei());
        }

        snreq.set_trace_id(sess->data().trace_id);

        if (bidreq.has_app())
            snreq.mutable_app_info()->CopyFrom(bidreq.app());
        if (bidreq.has_device())
            snreq.mutable_device_info()->CopyFrom(bidreq.device());

        // 拼请求sn的pb
        snreq.set_session_id(util::Func::to_str(sid));
        // dspid
        const rtb::BidRequest::Ext& bidreqext = bidreq.ext();
        snreq.set_dsp_id(bidreqext.dsp_id());
        // traffic_source
        snreq.set_traffic_source(bidreqext.traffic_source());
        // adzinfo
        rt = build_adz_info(sess);
        if (rt != 0) {
            LOG_ERROR("build_adz_info return error, rt[%d]", rt);
            rt = -1;
            break;
        }
        rt = build_time_info(sess);
        if (rt != 0) {
            LOG_ERROR("build_time_info error, rt[%d]\n", rt);
            rt = -1;
            break;
        }
        if (0 != build_geo(sess)) {
            LOG_ERROR("build_geo error, rt[%d]\n", rt);
            rt = -1;
            break;
        }

    } while (0);
    return rt;
}

int Scheduler::build_time_info(Session * sess) {
    int rt = 0;
    do {
        sn::SNRequest snreq = sess->data().snreq;

        sn::TimeInfo *time_info = snreq.mutable_time_info();
        // 获取当前时间戳
        time_t unix_seconds;
        time(&unix_seconds);
        struct tm* cur_tm = localtime(&unix_seconds);
        // 设置TimeInfo
        time_info->set_date_index(cur_tm->tm_yday + 1);
        time_info->set_timestamp(unix_seconds);
        time_info->set_hour(cur_tm->tm_hour);
        time_info->set_weekday(cur_tm->tm_wday);
        return 0;
    } while (0);
    return rt;
}

int Scheduler::build_geo(Session * sess) {
    int rt = 0;
    do {
        rtb::BidRequest & bidreq = sess->data().bidreq;
        sn::SNRequest & snreq = sess->data().snreq;
        if (!bidreq.has_device()) {
            break;
        }
        // ip
        const rtb::Device& rtbDevice = bidreq.device();
        common::Geo* geo = snreq.mutable_geo();
        SET_VALUE_PTR(geo, rtbDevice, ip, ip);
        // lat, lon
        if (!rtbDevice.has_geo()) {
            break;
        }
        const rtb::Geo& rtbGeo = rtbDevice.geo();
        SET_VALUE_PTR(geo, rtbGeo, latitude, lat);
        SET_VALUE_PTR(geo, rtbGeo, longtitude, lon);
    } while (0);
    return rt;
}

int Scheduler::build_adz_info(Session *sess) {
    int rt = 0;
    do {
        rtb::BidRequest & bidreq = sess->data().bidreq;
        sn::SNRequest & snreq = sess->data().snreq;
        qp::QPResponse & qprsp = sess->data().qprsp;

        std::set < uint32_t > bidTemplateIds;
        if (bidreq.has_app() && bidreq.app().has_ext()) {
            const rtb::App::Ext& bid_app_ext = bidreq.app().ext();
            int templateIdsSize = bid_app_ext.native_template_ids_size();
            for (int i = 0; i < templateIdsSize; ++i) {
                bidTemplateIds.insert(bid_app_ext.native_template_ids(i));
            }
        }

        // TODO: 确认广告位限制数 限制impression数
        int impSize =
                bidreq.impressions_size() > 10 ? 10 : bidreq.impressions_size();
        // dspid
//        const rtb::BidRequest::Ext& bid_req_ext = bidreq.ext();
//        const std::string& dspId = bid_req_ext.dsp_id();
        std::set < uint32_t > templateIds;
        std::map < std::string, uint32_t > &mincpmprice =
                sess->data().mapminprice;
        for (int i = 0; i < impSize; ++i) {
            const rtb::Impression &impression = bidreq.impressions(i);
            const std::string& impId = impression.id();
            // 设置底价
            mincpmprice[impId] = impression.bidfloor();

            sn::AdzInfo *adz = snreq.add_adz_info();
            // adzone id
            adz->set_id(impId);

            if (impression.has_video())
                adz->mutable_video()->CopyFrom(impression.video());
            if (impression.has_bidfloor())
                adz->set_min_cpm_price(impression.bidfloor());
            //pab直投
            if (impression.has_pmp() && impression.pmp().deals_size() > 0) {
                adz->set_deal_id(impression.pmp().deals(0).id());
            }

            // banner广告
            if (impression.has_banner()) {
                const rtb::Banner& banner = impression.banner();
                // 广告位尺寸
                adz->set_width(banner.width());
                adz->set_height(banner.height());
                //屏蔽的创意类型
                int blockedCreativeTypeSize =
                        banner.blocked_creative_types_size();
                for (int j = 0; j < blockedCreativeTypeSize; ++j) {
                    adz->add_blocked_creative_format(
                            banner.blocked_creative_types(j));
                }
            }
            // 视频广告
            else {
                const rtb::Video& video = impression.video();
                // 广告位尺寸
                adz->set_width(video.width());
                adz->set_height(video.height());
                if (video.has_ext()) {
                    const rtb::Video_Ext& ext = video.ext();
                    for (int j = 0; j < ext.blocked_creative_types_size();
                            ++j) {
                        adz->add_blocked_creative_format(
                                ext.blocked_creative_types(j));
                    }
                }
            }
            // template id
            std::set<uint32_t>::const_iterator iter = bidTemplateIds.begin();
            std::set<uint32_t>::const_iterator iter_end = bidTemplateIds.end();

            for (; iter != iter_end; ++iter) {
                adz->add_template_id(*iter);
            }

            //多view_type支持,兼容adapter和sn新老协议
            if (impression.view_types_size() > 0) {
                adz->set_view_type(impression.view_types(0));
                adz->mutable_view_types()->CopyFrom(impression.view_types());
            } else {
                if (impression.has_ext()) {
                    const rtb::Impression::Ext& impExt = impression.ext();
                    if (impExt.has_view_type()) {
                        adz->set_view_type(impExt.view_type());
                        adz->add_view_types(impExt.view_type());
                    }
                }
            }

            if (impression.has_ext()) {
                const rtb::Impression::Ext& impExt = impression.ext();
                // 设置创意等级
                if (impExt.has_allowed_creative_level()) {
                    adz->set_creative_level(impExt.allowed_creative_level());
                }
                if (impExt.has_ad_num()) {
                    adz->set_ad_num(impExt.ad_num());
                }

                int excludedSenstiveCateSize =
                        impExt.excluded_sensitive_category_size();
                for (int j = 0; j < excludedSenstiveCateSize; ++j) {
                    adz->add_blocked_creative_category(
                            impExt.excluded_sensitive_category(j));
                }
                // TODO 是不是要过滤
                // 禁止的广告行业类目
                int excludedAdCateSize = impExt.excluded_ad_category_size();
                for (int j = 0; j < excludedAdCateSize; ++j) {
                    adz->add_blocked_creative_category(
                            impExt.excluded_ad_category(j));
                }
            }
            // 定向
            if (qprsp.has_dmp_info()) {
                const qp::DmpInfo dmpinfo = qprsp.dmp_info();
                int UserTargetSize = dmpinfo.user_targets_size();
                if (UserTargetSize > 0) {
                    google::protobuf::RepeatedPtrField<common::Targetting>* targetting =
                            adz->mutable_targetting();
                    targetting->CopyFrom(dmpinfo.user_targets());
                }
            }
        } // for

        return 0;
    } while (0);
    return rt;
}

}
}
