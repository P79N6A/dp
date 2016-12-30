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
#include <boost/unordered_map.hpp>

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

/**
 * @brief             called when get Ors Rsp after 
 **/
int Scheduler::process_ors_get(Session * sess) {
    int rt = 0;
    do {
        sess->data().prog |= PROG_ORS_RSP;
        if (!enable_backup_ad(sess)) {
            if (sess->data().orsrsp.algoed_ads_size() == 0) {
                LOG_DEBUG("orsrsp algoed ads size is 0");
                sess->status(STAT_FAIL);
                process_fail(sess);
                break;
            }
        }
        rt = build_adapter_rsp(sess);
        if (rt != 0) {
            LOG_ERROR("build_adapter_rsp error, rt[%d]\n", rt);
            rt = -1;
            sess->free();
            break;
        }
        rt = ProcessAdapter::get_mutable_instance().response_client(sess);
        if (rt != 0) {
            LOG_ERROR("ProcessDn query return error, rt[%d]\n", rt);
            rt = -1;
            sess->free();
            break;
        }
        MON_ADD(ATTR_SUCC_RSP, 1);
        sess->data().prog |= PROG_ADAPTER_RSP;
        write_log(sess);
        sess->free();
    } while (0);
    return rt;
}

int Scheduler::build_ors_req(Session *sess) {
    int rt = 0;
    do {
        const rtb::BidRequest & bidreq = sess->data().bidreq;
        const qp::QPResponse & qprsp = sess->data().qprsp;
        const sn::SNResponse & snrsp = sess->data().snrsp;
        const feedback::FeedbackResponse & fbrsp = sess->data().fbrsp;
//        const dn::DNResponse & dnrsp=sess->data().dnrsp;
        ors::AlgoRequest & orsreq = sess->data().orsreq;

        orsreq.set_trace_id(sess->data().trace_id);

        int sid = sess->sid();
        orsreq.set_session_id(util::Func::to_str(sid));

        if (snrsp.has_scoring_to_ors_msg())
            orsreq.mutable_scoring_to_ors_msg()->CopyFrom(
                    snrsp.scoring_to_ors_msg());

        //dsp id
        bool dspIdIsEmpty = true;
        if (bidreq.has_ext()) {
            if (bidreq.ext().has_dsp_id()) {
                orsreq.set_dspid(bidreq.ext().dsp_id());
                dspIdIsEmpty = false;
            }
        }
        if (dspIdIsEmpty) {
            rt = -1;
            break;
        }

        // bidid
        orsreq.set_bidid(bidreq.id());
        
        if(fbrsp.has_user_source_freq())
        {
          orsreq.set_user_source_freq(fbrsp.user_source_freq());
        }
        if(fbrsp.has_user_source_click_freq())
        {
          orsreq.set_user_source_click_freq(fbrsp.user_source_click_freq());
        }

        rt = ors_build_traffic_info(sess);
        if (rt != 0) {
            break;
        }
        // user info
        rt = ors_build_user_info(sess);
        if (rt != 0) {
            break;
        }

        // device info
        rt = ors_build_device_info(sess);
        if (rt != 0) {
            break;
        }
        // ad info
        rt = ors_build_algo_ad(sess);
        if (rt != 0) {
            break;
        }
        if (bidreq.has_app()) {
            orsreq.mutable_app_info()->CopyFrom(bidreq.app());
        }

        if (orsreq.algo_ads_size() == 0) {
            sess->status(STAT_FAIL);
            process_fail(sess);
            rt = -2;
            MON_ADD(ATTR_NO_ALGO_ADS, 1);
            break;
        }

        if (qprsp.has_dmp_info()) {
            const qp::DmpInfo & dmpinfo = qprsp.dmp_info();
            int user_tag_size = dmpinfo.user_targets_size();
            for (int i = 0; i < user_tag_size; i++) {
                common::Targetting * ptag = orsreq.add_targets();
                ptag->CopyFrom(dmpinfo.user_targets(i));
            }
            int ors_tag_size = dmpinfo.ors_targets_size();
            for (int i = 0; i < ors_tag_size; i++) {
                common::Targetting * ptag = orsreq.add_targets();
                ptag->CopyFrom(dmpinfo.ors_targets(i));
            }
        }

        // video
        if (bidreq.impressions_size() > 0) {
            const rtb::Impression& imp = bidreq.impressions(0);
            if (imp.has_video()) {
                rtb::Video* video = orsreq.mutable_video();
                video->CopyFrom(imp.video());
            }
        }

        std::set < uint32_t > &setfilter = sess->data().setfilter;
        int size = fbrsp.feedbackinfo_size();
        for (int i = 0; i < size; i++) {
            const common::FeedbackInfo & fbinfo = fbrsp.feedbackinfo(i);
            if (setfilter.count(fbinfo.adgroup_id()) > 0) {
                continue;
            }
            common::FeedbackInfo * pfeedback = orsreq.add_feedbacks();
            pfeedback->CopyFrom(fbrsp.feedbackinfo(i));
        }

        ors_build_exp(sess);
    } while (0);
    return rt;
}

int Scheduler::ors_build_traffic_info(Session * sess) {
    const rtb::BidRequest& bidreq = sess->data().bidreq;
    ors::AlgoRequest & orsreq = sess->data().orsreq;
    ors::TrafficInfo* trafficInfo = orsreq.mutable_traffic_info();
    const rtb::Site& rtbSite = bidreq.site();
    SET_VALUE_PTR(trafficInfo, rtbSite, domain, domain);
    SET_VALUE_PTR(trafficInfo, rtbSite, site, name);
    SET_VALUE_PTR(trafficInfo, rtbSite, url, page);
    SET_VALUE_PTR(trafficInfo, rtbSite, ref, ref);

    const rtb::BidRequest::Ext& rtbExt = bidreq.ext();
    SET_VALUE_PTR(trafficInfo, rtbExt, traffic_source, traffic_source);

    int siteCatSize = rtbSite.site_categories_size();
    for (int i = 0; i < siteCatSize; ++i) {
        trafficInfo->add_site_categories(rtbSite.site_categories(i));
    }

    int sectionCatSize = rtbSite.sectioncat_size();
    for (int i = 0; i < sectionCatSize; ++i) {
        trafficInfo->add_sectioncat(rtbSite.sectioncat(i));
    }

    if (rtbSite.has_ext()) {
        int pageCatSize = rtbSite.ext().page_category_size();
        for (int i = 0; i < pageCatSize; ++i) {
            const rtb::Site::Ext::PageCategory& rtbPageCat =
                    rtbSite.ext().page_category(i);
            ors::PageCategory* pageCat = trafficInfo->add_page_category();
            pageCat->set_id(rtbPageCat.id());
            pageCat->set_weight(rtbPageCat.weight());
        }
    }

    if (bidreq.has_app()) {
        const rtb::App& rtbApp = bidreq.app();
        trafficInfo->set_app_name(rtbApp.bundle());
        // TODO 用的是app的keywords
        int keywordSize = rtbApp.keywords_size();
        for (int i = 0; i < keywordSize; ++i) {
            trafficInfo->add_keywords(rtbApp.keywords(i));
        }
    }

    // TODO ors只处理一个impression
    if (bidreq.impressions_size() > 0) {
        const rtb::Impression& rtbImp = bidreq.impressions(0);
        trafficInfo->set_imp_id(rtbImp.id());
        if (rtbImp.has_ext()) {
            const rtb::Impression_Ext& imprExt = rtbImp.ext();
            if (imprExt.has_ad_num()) {
                trafficInfo->set_ad_num(imprExt.ad_num());
            }
            if (rtbImp.ext().has_view_screen()) {
                trafficInfo->set_view_screen(rtbImp.ext().view_screen());
            }
        }
        if (rtbImp.has_banner()) {
            const rtb::Banner& rtbBanner = rtbImp.banner();
            SET_VALUE_PTR(trafficInfo, rtbBanner, width, width);
            SET_VALUE_PTR(trafficInfo, rtbBanner, height, height);
        }

        //多view_type支持，兼容新老协议
        if (rtbImp.view_types_size() > 0) {
            trafficInfo->set_view_type(rtbImp.view_types(0));
            trafficInfo->mutable_view_types()->CopyFrom(rtbImp.view_types());
        } else {
            if (rtbImp.has_ext()) {
                const rtb::Impression_Ext& imprExt = rtbImp.ext();
                if (imprExt.has_view_type()) {
                    trafficInfo->set_view_type(imprExt.view_type());
                    trafficInfo->add_view_types(imprExt.view_type());
                }
            }
        }

        if (rtbImp.has_bidfloor()) {
            trafficInfo->set_min_cpm_price(rtbImp.bidfloor());
        }

    }

    return 0;
}

int Scheduler::ors_build_user_info(Session * sess) {
    const rtb::BidRequest& bidreq = sess->data().bidreq;
    const qp::QPResponse& qprsp = sess->data().qprsp;
    ors::AlgoRequest & orsreq = sess->data().orsreq;

    ors::UserInfo* userInfo = orsreq.mutable_user_info();

    // from request of rtb
    if (bidreq.has_user() && bidreq.user().has_ext()) {
        const rtb::User& rtbUser = bidreq.user();
        SET_VALUE_PTR(userInfo, rtbUser.ext(), acookie, acookie);
        SET_VALUE_PTR(userInfo, rtbUser.ext(), aid, aid);
    }
    // from response of qp
    if (!qprsp.has_dmp_info()) {
        return 0;
    }
//    TODOFIX("fix dmpinfo");

    return 0;
}

int Scheduler::ors_build_device_info(Session * sess) {
    const rtb::BidRequest& bidreq = sess->data().bidreq;
    ors::AlgoRequest & orsreq = sess->data().orsreq;
    rtb::Device* deviceInfo = orsreq.mutable_device_info();
    if (!bidreq.has_device()) {
        return 0;
    }
    const rtb::Device& rtbDevice = bidreq.device();
    deviceInfo->CopyFrom(rtbDevice);
    return 0;
}

int Scheduler::ors_build_algo_ad(Session * sess) {
//    const sn::SNResponse& snrsp=sess->data().snrsp;
    const rtb::BidRequest & bidreq = sess->data().bidreq;

    dn::DNResponse& dnrsp = sess->data().dnrsp;
    const sn::SNResponse & snrsp = sess->data().snrsp;
    boost::unordered_map<uint64_t, uint32_t> creative_map;
    ors::AlgoRequest& orsreq = sess->data().orsreq;

    TODOFIX("只处理一个广告位，先忽略多个广告位的情况");

    if (bidreq.impressions_size() > 0) {
        const rtb::Impression & impr = bidreq.impressions(0);

        //微博,爱奇艺广告和头条广告,不做匹配，全部放过
        if (sess->data().traffic_source == rtb::TS_IQIYI
                || sess->data().traffic_source == rtb::TS_TOUTIAO
                || sess->data().traffic_source == rtb::TS_WAX) {
            int csize = dnrsp.creative_size();
            for (int cidx = 0; cidx < csize; cidx++) {
                creative_map[dnrsp.creative(cidx).creative_id()] = cidx;
            }
        }
        //针对yunos请求做过滤
        else if (sess->data().traffic_source == rtb::TS_YUNOS) {
            Json::Reader reader;

            std::set < std::string > black_pkgs;
            std::set < std::string > white_pkgs;
            Json::Value yunos_categories;

            if (bidreq.has_ext() && bidreq.ext().has_request_json()) {
                std::string request_json_str = bidreq.ext().request_json();
                Json::Value request_json;
                if (reader.parse(request_json_str, request_json)) {
                    Json::Value excluedpkgs = request_json["excluedpkgs"];
                    if (excluedpkgs.isArray()) {
                        for (int i = 0; i < excluedpkgs.size(); i++) {
                            if (excluedpkgs[i].isString()) {
                                black_pkgs.insert(excluedpkgs[i].asString());
                            }
                        }
                    }
                }
            }
            if (impr.has_ext() && impr.ext().has_special_json()) {
                /*举例
                 {"categories":["1","2"], "pkgs":["1", "2"], "keyword":["1", "2"], "postype":"LIST"}
                 */
                std::string special_json_str = impr.ext().special_json();
                //LOG_INFO("yunos request : %s",bidreq.DebugString().c_str());
                //LOG_INFO("yunos special_json : %s",special_json_str.c_str());
                Json::Value special_json;
                if (reader.parse(special_json_str, special_json)) {
                    yunos_categories = special_json["categories"];
                    Json::Value pkgs_obj = special_json["pkgs"];
                    if (pkgs_obj.isArray()) {
                        for (int i = 0; i < pkgs_obj.size(); i++) {
                            if (pkgs_obj[i].isString()
                                    && pkgs_obj[i].asString().length() > 0)
                                white_pkgs.insert(pkgs_obj[i].asString());
                        }
                    }
                } else {
                    LOG_ERROR("yunos special_json parse error,",
                            special_json_str.c_str());
                }
            }
            int csize = dnrsp.creative_size();
            int cidx;
            for (cidx = 0; cidx < csize; cidx++) {
                bool mapped = false;
                const common::Creative & c = dnrsp.creative(cidx);
                if (!yunos_categories.isNull()) {
                    if (yunos_categories.isArray()) {
                        for (int no = 0; no < yunos_categories.size(); no++) {
                            if (yunos_categories[no].isString()) {
                                mapped =
                                        YunosGameMap::get_mutable_instance().is_mapped(
                                                yunos_categories[no].asString(),
                                                c);
                                if (mapped)
                                    break;
                            } else {
                                LOG_DEBUG(
                                        "yunos game no mapped,json is not string or array");
                                continue;
                            }
                            if (!mapped) {
                                LOG_DEBUG(
                                        "yunos game no mapped,yunos_game_map retrun false");
                                continue;
                            }
                        }
                    } else {
                        LOG_ERROR("yunos categories is not array");
                        continue;
                    }
                } else {
                    LOG_ERROR("yunos categories is null");
                    mapped = true;
                }

                LOG_INFO("yunos mapped : %d,creative is \r\n %s", mapped,
                        c.DebugString().c_str());
                if (!mapped)
                    continue;

                if (c.has_specific_data()) {
                    std::string specific_data_str = c.specific_data();
                    Json::Value special_data;
                    if (reader.parse(specific_data_str, special_data)) {
                        Json::Value pkg_name_obj = special_data["packagename"];
                        if (pkg_name_obj.isString()) {
                            std::string pkg_name = pkg_name_obj.asString();
                            if (black_pkgs.count(pkg_name) > 0) {
                                LOG_DEBUG("yunos bid is in black");
                                continue;
                            }
                            if (white_pkgs.size() > 0
                                    && white_pkgs.count(pkg_name) == 0) {
                                LOG_DEBUG(
                                        "yunos bid pkg_name[%s]is not in white",
                                        pkg_name.c_str());
                                continue;
                            }
                        } else
                            continue;
                    }
                }
                creative_map[c.creative_id()] = cidx;
            }
        } else {
            if (impr.has_banner()) {
                const rtb::Banner & banner = impr.banner();
                size_t impr_height = banner.height();
                size_t impr_witch = banner.width();

                int native_flag = 0;
                std::vector<TemplateInfo> vrtemp;
                if (bidreq.has_app() && bidreq.app().has_ext()
                        && bidreq.app().ext().native_template_ids_size() > 0) { //信息流数据
                                                                                //获取允许的模板
                    int temp_size =
                            bidreq.app().ext().native_ad_template_size();
                    for (int i = 0; i < temp_size; i++) {
                        TemplateInfo ti;
                        const rtb::App::Ext::NativeAdTemplate & nt =
                                bidreq.app().ext().native_ad_template(i);
                        ti.template_id = nt.native_template_id();
                        ti.width = nt.w();
                        ti.height = nt.h();
                        vrtemp.push_back(ti);
                    }
                    native_flag = 1;
                }

                int csize = dnrsp.creative_size();
                int cidx;
                for (cidx = 0; cidx < csize; cidx++) {
                    common::Creative * c = dnrsp.mutable_creative(cidx);
                    if (native_flag) {                  //信息流数据
                        std::vector<TemplateInfo>::iterator it;
                        for (it = vrtemp.begin(); it != vrtemp.end(); it++) {
                            if (it->width == c->width()
                                    && it->height == c->height()) {
                                c->set_template_id(it->template_id);
                                creative_map[c->creative_id()] = cidx;
                                break;
                            }
                        }

                    } else if (c->width() == impr_witch
                            && c->height() == impr_height) {
                        creative_map[c->creative_id()] = cidx;
                    }
                }
            } else if (impr.has_video()) {
                const rtb::Video & video = impr.video();
                FormatFilter ff;

                int fmt_size = video.formats_size();
                if (fmt_size > 0) {                  //每个格式不限制
                    for (int i = 0; i < fmt_size; i++) {
                        ff.add_allow_format(video.formats(i));
                    }
                }

                int csize = dnrsp.creative_size();
                int cidx;
                for (cidx = 0; cidx < csize; cidx++) {
                    common::Creative * c = dnrsp.mutable_creative(cidx);
                    if (fmt_size > 0 && c->has_suffix()) {
                        if (!ff.allow(c->suffix())) {
                            continue;
                        }
                    }
                    creative_map[c->creative_id()] = cidx;

                }
            } else {
                /*保存*/
                int csize = dnrsp.creative_size();
                int cidx;
                for (cidx = 0; cidx < csize; cidx++) {
                    const common::Creative & c = dnrsp.creative(cidx);
                    creative_map[c.creative_id()] = cidx;
                }
            }
        }
    }

    {
        std::set < uint32_t > &setfilter = sess->data().setfilter;
        int adsidx = snrsp.ads_size();
        if (adsidx < 1) {
            return -1;
        }
        const sn::Ads & ads = snrsp.ads(0);
        int size_ad = ads.ad_size();
        int aidx;
        int ididx = 0;
        for (aidx = 0; aidx < size_ad; aidx++) {
            const common::Ad & ad = ads.ad(aidx);
            if (creative_map.count(ad.creative_id()) == 0) {
                setfilter.insert(ad.adgroup_id());
                continue;
            }
            if (setfilter.count(ad.adgroup_id()) > 0) {
                continue;
            }

            ors::AlgoAd * algo_ad = orsreq.add_algo_ads();
            algo_ad->set_id(ididx++);
            common::Ad * pad = algo_ad->mutable_ad();
            pad->CopyFrom(ad);
            boost::unordered_map<uint64_t, uint32_t>::iterator iter =
                    creative_map.find(ad.creative_id());
            if (iter == creative_map.end())
                continue;
            const common::Creative & cr = dnrsp.creative(iter->second);
            common::Creative * pcr = algo_ad->mutable_creative();
            pcr->CopyFrom(cr);
        }
    }

    return 0;
}

int Scheduler::ors_build_exp(Session * sess) {
    const rtb::BidRequest & bidreq = sess->data().bidreq;
    ors::AlgoRequest& orsreq = sess->data().orsreq;

    int source = sess->data().traffic_source;
    if (bidreq.impressions_size() > 0 && bidreq.impressions(0).has_ext()
            && bidreq.impressions(0).ext().has_view_type()) {

    } else
        return 0;
    const rtb::Impression & impr = bidreq.impressions(0);
    const rtb::Impression::Ext& impExt = impr.ext();

    std::vector < int32_t > view_types;
    if (impr.view_types_size() > 0) {
        for (int i = 0; i < impr.view_types_size(); i++)
            view_types.push_back(impr.view_types(i));
    } else {
        view_types.push_back(impExt.view_type());
    }

    for (int i = 0; i < view_types.size(); i++) {
        int exp_ret = exp_sys::ExpApi::get_mutable_instance().get_exp_param(
                CONTROL_EXP_MODULE_ID, source, view_types[i], sess->data().vr_exp_id,
                sess->data().vr_exp_param);
        
        std::stringstream pv_exp_id;
        for(int j=0;j<sess->data().vr_exp_id.size();j++)
          pv_exp_id<<sess->data().vr_exp_id[i]<<"|";
        
        LOG_DEBUG(
                "module_id=%d`source=%d`view_type=%d`vr_exp_id=%s`exp_param=%d`exp_ret=%d",
                CONTROL_EXP_MODULE_ID, source, view_types[i], pv_exp_id.str().c_str(),
                sess->data().vr_exp_param.size(), exp_ret);

        if (exp_ret == 0) {
            for (int p = 0; p < sess->data().vr_exp_param.size(); p++) {
                common::ExpParam *exp_param = orsreq.add_exp_param();
                exp_param->set_view_type(view_types[i]);
                exp_param->set_param_id(sess->data().vr_exp_param[p].param_id);
                if (sess->data().vr_exp_param[p].param_type
                        == exp_sys::PT_INT) {
                    exp_param->set_int_value(
                            sess->data().vr_exp_param[p].param_vlaue.int_v);
                } else if (sess->data().vr_exp_param[p].param_type
                        == exp_sys::PT_FLOAT) {
                    exp_param->set_float_value(
                            sess->data().vr_exp_param[p].param_vlaue.float_v);
                } else {

                }
            }
        }
    }
    return 0;
}

}
}
