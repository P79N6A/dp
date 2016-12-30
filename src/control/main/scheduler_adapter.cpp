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

namespace poseidon {
namespace control {

/**
 * @brief           called when get Adapter Req after
 **/
int Scheduler::process_adapter_get(Session * sess) {
    ++serial_num_;
    int rt = 0;
    do {
        sess->data().prog |= PROG_ADAPTER_REQ;
        const rtb::BidRequest & bidreq = sess->data().bidreq;
        rtb::BidResponse & bidrsp = sess->data().bidrsp;

        if (bidreq.has_trace_id()) {
            bidrsp.set_trace_id(bidreq.trace_id());
            sess->data().trace_id = bidreq.trace_id();
        } else {
            char tmp[1024] = { 0 };
            sprintf(tmp, "%lu%d%d", serial_num_, pthread_self(),
                    time((time_t) NULL));
            util::Func::md5sum(tmp, strlen(tmp), sess->data().trace_id);
        }


        if (bidreq.has_ext() && bidreq.ext().has_traffic_source()) {
            sess->data().traffic_source = bidreq.ext().traffic_source();
            if (sess->data().traffic_source > 8
                    || sess->data().traffic_source < 0) {
                LOG_ERROR("error traffic_source : %d",
                        sess->data().traffic_source);
            }
        } else {
            sess->data().traffic_source = 0;
        }

//控制QPS流量
        if (!qpscontrol_.allow()) {
            MON_ADD(ATTR_QPS_CONTROL_CUT, 1);
            sess->status(STAT_FAIL);
            process_fail(sess);
            rt = -1;
            break;
        }
//配置强制不竞价
        if (Config::get_mutable_instance().no_bid(
                sess->data().traffic_source)) {
            sess->status(STAT_FAIL);
            process_fail(sess);
            rt = -1;
            break;
        }

//只竞价安卓系统
        if ((!bidreq.has_device())
                || (bidreq.device().os() != "android"
                        && bidreq.device().os() != "yunos")) {
            sess->status(STAT_FAIL);
            process_fail(sess);
            rt = -1;
            break;
        }

//过来优土 pad广告
        if (bidreq.device().has_device_type()
                && bidreq.device().device_type() == 3
                && sess->data().traffic_source == rtb::TS_YOUTU) {
            sess->status(STAT_FAIL);
            process_fail(sess);
            rt = -1;
            break;
        }

//            PV_SET(os, bidreq.device().os());

        rt = build_qp_req(sess);
        if (rt != 0) {
            LOG_ERROR("build_qp_req error, rt[%d]\n", rt);
            rt = -1;
            break;
        }
        util::Func::get_time_ms(sess->data().time_qpreq);
        rt = ProcessQp::get_mutable_instance().query(sess->data().qpreq);
        if (rt != 0) {
            LOG_ERROR("ProcessQp query return error, rt[%d]", rt);
            rt = -1;
            break;
        }
        sess->data().prog |= PROG_QP_REQ;

    } while (0);
    return rt;
}

int Scheduler::build_adapter_rsp(Session * sess) {
    int rt = 0;
    do {
        rtb::BidResponse & bidrsp = sess->data().bidrsp;
        rtb::BidRequest & bidreq = sess->data().bidreq;
        ors::AlgoRequest & orsreq = sess->data().orsreq;
        ors::AlgoResponse & orsrsp = sess->data().orsrsp;

        bidrsp.set_id(bidreq.id());
        if (orsrsp.error_code() != common::ERROR_NONE
                && orsrsp.error_code() != common::ERROR_BACKUP_AD) {
            bidrsp.set_no_bid_reason(NBR_TECHNICAL_ERROR);
            break;
        }

        if(bidreq.has_device() && bidreq.device().has_id())
        {
          bidrsp.set_dev_id(bidreq.device().id());
        }
        int ts = TANX;
        if (bidreq.has_ext() && bidreq.ext().has_traffic_source()) {
            ts = bidreq.ext().traffic_source();
        }
        if (ts == JS_PT) {
            LOG_ERROR("traffic_source is JS_PT\n");
            bidrsp.set_no_bid_reason(NBR_TECHNICAL_ERROR);
            break;
        }


        const rtb::Impression & impr = bidreq.impressions(0);
        if (!impr.has_ext()) {
            LOG_ERROR("Impression Ext isn't existed\n");
            bidrsp.set_no_bid_reason(NBR_TECHNICAL_ERROR);
            break;
        }

        const rtb::Impression::Ext & impr_ext = impr.ext();
        if (!impr_ext.has_ad_num()) {
            LOG_ERROR("Impression Ext !has_ad_num\n");
            bidrsp.set_no_bid_reason(NBR_TECHNICAL_ERROR);
            break;
        }

        if (orsrsp.algo_feedbacks_size() > 0) {
            bidrsp.mutable_algo_feedbacks()->CopyFrom(orsrsp.algo_feedbacks());
        }

        int ors_ads_size = orsrsp.algoed_ads_size();
        if (ors_ads_size <= 0) {

            if (enable_backup_ad(sess)) {
                int backup_ad_idx = 0;
                ors::AlgoAd algo_ad = orsreq.algo_ads(backup_ad_idx);
                common::Ad ad = algo_ad.ad();

                ors::AlgoedAd * algoed_ad = orsrsp.add_algoed_ads();
                algoed_ad->set_id(backup_ad_idx);
                if (ad.has_org_price()) {
                    algoed_ad->set_algo_price(ad.org_price());
                    algoed_ad->set_cost_price(ad.org_price());
                } else {
                    algoed_ad->set_algo_price(0);
                    algoed_ad->set_cost_price(0);
                }
                sess->data().enabled_backup_ad = true;
                sess->status(STAT_ENABLE_BACKUP_AD);
            } else {
                LOG_ERROR("orsrsp.algoed_ads_size <= 0\n");
                bidrsp.set_no_bid_reason(NBR_TECHNICAL_ERROR);
                break;
            }
        }

        ors_ads_size = orsrsp.algoed_ads_size();
        int req_ad_num = impr_ext.ad_num();
        int orsreq_ads_size = orsreq.algo_ads_size();
        LOG_DEBUG("trace_id[%s],ors_ads_size = %d",
                sess->data().trace_id.c_str(), ors_ads_size);
        LOG_DEBUG("trace_id[%s],req_ad_num = %d", sess->data().trace_id.c_str(),
                req_ad_num);
        for (int i = 0; i < ors_ads_size && i < req_ad_num; i++) {
            const ors::AlgoedAd & algoedad = orsrsp.algoed_ads(i);
            if (!algoedad.has_algo_price()) {
                LOG_DEBUG("algoedad[%s] !has_algo_price()",
                        algoedad.DebugString().c_str());
                continue;
            }

            int32_t algo_price = algoedad.algo_price();
            if (algo_price <= 0) {
                LOG_DEBUG("algoedad[%s] algo_price <= 0",
                        algoedad.DebugString().c_str());
                continue;
            }

            rtb::BidSeat * pbidseat = NULL;
            if (bidrsp.bid_seats_size() == 0) {
                pbidseat = bidrsp.add_bid_seats();
            } else {
                pbidseat = bidrsp.mutable_bid_seats(0);
            }

            int idx = algoedad.id();
//            uint64_t cid=orsreq.algo_ads(idx).ad().creative_id();
//            common::Creative & c=sess->data().mapcreative[cid];
            if (idx > orsreq_ads_size) {
                LOG_ERROR("error idx[%d]orsreq_ads_size[%d]", idx,
                        orsreq_ads_size);
                rt = -1;
                break;
            }
            const common::Ad & ad = orsreq.algo_ads(idx).ad();
            const common::Creative & c = orsreq.algo_ads(idx).creative();
            uint64_t cid = c.creative_id();

            rtb::Bid * pbid = pbidseat->add_bids();


            if (ad.has_view_type()) {
                pbid->set_view_type(ad.view_type());
            } else if (impr_ext.has_view_type()) {
                pbid->set_view_type(impr_ext.view_type());
            }else if(impr.view_types_size()>0)
            {
                pbid->set_view_type(impr.view_types(0));
            }

            if (ad.has_premium_rate()) {
                pbid->set_premium_rate(ad.premium_rate());
            }


            if (c.has_ext_cid()) {
                pbid->set_ext_cid(c.ext_cid());
            }


            if (ad.has_ch()) {
                pbid->set_ch(ad.ch());
            }
            if (ad.has_inner_advertiser_id()) {
                pbid->set_inner_advertiser_id(ad.inner_advertiser_id());
            }
            if (ad.has_gid()) {
                pbid->set_gid(ad.gid());
            }

            char bidid[5];
            snprintf(bidid, 5, "%u", i);
            pbid->set_id(bidid);
            pbid->set_impid(bidreq.impressions(0).id());


            if (c.has_specific_data()) {
                pbid->set_specific_data(c.specific_data());
                LOG_DEBUG("copy specific data,creative id=%ld", cid);
            }

            if (algoedad.has_traffic_bid_flag()) {
                pbid->set_traffic_bid_flag(algoedad.traffic_bid_flag());
            }

            if (Config::get_mutable_instance().bid_one_cent(
                    sess->data().traffic_source)) {
                pbid->set_price(1);
            } else {
                pbid->set_price(algo_price);
            }
            //ad_id
            //notice_utl
            //ad_tag
            //adm
            if (impr.has_ext() && impr.ext().has_view_type()) {
//                int view_type=impr.ext().view_type();
                if (c.has_img_url()) {
                    pbid->set_image_url(c.img_url());
                }
                if (c.has_video_url()) {
                    pbid->set_adm(c.video_url());
                }
            }
            //advertiser_domain
            pbid->set_image_url(c.content());
            pbid->set_campaign_id(ad.campaign_id());

            char scid[32];
            snprintf(scid, 32, "%lu", cid);
            pbid->set_creative_id(scid);
            //deal_id
            if (ad.has_pdb_data() && ad.pdb_data().has_deal_id()) {
                pbid->set_deal_id(ad.pdb_data().deal_id());
                if (pbid->has_ext() && ad.pdb_data().has_settle_price()) {
                    pbid->mutable_ext()->set_settle_price(
                            ad.pdb_data().settle_price());
                }
            }

            pbid->set_w(c.width());
            pbid->set_h(c.height());

            if (c.has_img_url()) {
                pbid->set_image_url(c.img_url());
            }

            //creative_attributes
            //

            rtb::Bid::Ext * pbidext = pbid->mutable_ext();


            int c_cate_size = c.creative_category_size();
            for (int i = 0; i < c_cate_size; i++) {
                pbidext->add_category(c.creative_category(i));
            }


            pbidext->set_brand_id(c.creative_brand_id());


            pbidext->set_creative_format(c.creative_format());


//            optional string creative_template_id = 4;

            char stempid[32];
            snprintf(stempid, 32, "%u", c.template_id());
            pbidext->set_creative_template_id(stempid);


            pbidext->set_dest_url(c.dest_url());


            pbidext->set_creative_level(c.creative_level());

            // click url
//            optional string click_url = 7;
            pbidext->set_click_url(c.click_url());

            if (c.has_ad_words()) {
                pbidext->set_ad_words(c.ad_words());
            }
            if (c.has_open_type()) {
                pbidext->set_open_type(c.open_type());
            }
            if (c.has_download_type()) {
                pbidext->set_download_type(c.download_type());
            }
            if (c.has_deeplink_url()) {
                pbidext->set_deeplink_url(c.deeplink_url());
            }
            if (c.has_title()) {
                pbidext->set_title(c.title());
            }

            if (c.has_landing_mode() && c.landing_mode() == 3) {
                if (c.has_click_url()) {
                    pbidext->set_download_url(c.click_url());
                }
            }


            char sadgrid[32];
            snprintf(sadgrid, 32, "%u", ad.adgroup_id());
            pbidext->set_adgroup_id(sadgrid);

            if (ad.has_freq_impression()) {
                pbidext->set_freq_impression(ad.freq_impression());
            }


//            optional uint32 advertiser_id = 11;
            pbidext->set_advertiser_id(ad.advertiser_id());


//            optional string billing_type = 12;
            pbidext->set_billing_type(ad.billing_type());


//            optional int32 settle_price = 13;
//            pbidext->set_settle_price(ad.org_price());
            if (algoedad.has_cost_price()) {
                pbidext->set_cost_price(algoedad.cost_price());
            } else {
                pbidext->set_cost_price(ad.org_price());
            }


//            optional int32 org_price = 15;
            pbidext->set_org_price(ad.org_price());


//            optional int64 campaign_daily_budget = 17;
            pbidext->set_campaign_daily_budget(ad.campaign_daily_budget());

//            optional SendSpeedType send_speed = 18;
            pbidext->set_send_speed(ad.send_speed());


//            optional uint32 post_hours = 19;
            pbidext->set_post_hours(ad.post_hours());

//            optional uint32 advertiser_budget = 20;
            pbidext->set_advertiser_budget(ad.advertiser_budget());

        }
        bidrsp.set_no_bid_reason(NBR_SUCCESS);
        bidrsp.set_bid_currency("CNY");
        if (bidreq.has_session_id()) {
            bidrsp.set_session_id(bidreq.session_id());
        }

        if (sess->data().vr_exp_id.size() > 0) {
            for (int p = 0; p < sess->data().vr_exp_id.size(); p++) {
                bidrsp.add_exp_id(sess->data().vr_exp_id[p]);
            }
        }

    } while (0);
    return rt;
}

int Scheduler::build_error_adapter_rsp(Session * sess) {
    int rt = 0;
    do {
        rtb::BidResponse & bidrsp = sess->data().bidrsp;
        rtb::BidRequest & bidreq = sess->data().bidreq;

        bidrsp.set_id(bidreq.id());
        bidrsp.set_no_bid_reason(NBR_TECHNICAL_ERROR);

    } while (0);
    return rt;
}

}
}
