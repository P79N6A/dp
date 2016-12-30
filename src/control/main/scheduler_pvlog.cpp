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

namespace poseidon {
namespace control {

void Scheduler::write_log(Session * sess) {
    const SessionData & sessdata = sess->data();
    const rtb::BidRequest & bidreq = sessdata.bidreq;
    const rtb::BidResponse & bidrsp = sessdata.bidrsp;
    const qp::QPResponse & qprsp = sessdata.qprsp;
    const sn::SNResponse & snrsp = sessdata.snrsp;
    const fc::FilterResponse & filterrsp = sessdata.filterrsp;
    const dn::DNResponse & dnrsp = sessdata.dnrsp;
    const ors::AlgoResponse & orsrsp = sessdata.orsrsp;
    std::stringstream ss;

#define PV_SET(name, val) ss<<(#name)<<"="<<(val)<<"`"

    uint64_t now;

    util::Func::get_time_ms(now);

    if (sess->status() == STAT_TIME_OUT) {
        PV_SET(pv_status, 1); //1, 超时
        PV_SET(prog, sessdata.prog);
    } else if (sess->status() == STAT_FAIL) {
        PV_SET(pv_status, 2); //2, 超时
        PV_SET(prog, sessdata.prog);
    } else {
        PV_SET(pv_status, 0); //2, 超时
        PV_SET(prog, sessdata.prog);
    }

    PV_SET(ver, "1.0");
    PV_SET(t, now);

    PV_SET(trace_id, sess->data().trace_id);

    PV_SET(source, sessdata.traffic_source);

    std::stringstream pv_exp_id;
    for (int i = 0; i < sessdata.vr_exp_id.size(); i++) {
        pv_exp_id << sessdata.vr_exp_id[i] << "|";
    }
    for (int i = 0; i < snrsp.exp_id_size(); i++) {
        pv_exp_id << snrsp.exp_id(i) << "|";
    }
    if (pv_exp_id.str().length() > 0)
        PV_SET(exp_id, pv_exp_id.str());
    std::stringstream pv_deal_id;
    if (bidreq.impressions_size() > 0) {
        const rtb::Impression &imp = bidreq.impressions(0);
        if (imp.has_pmp()) {
            if (imp.pmp().deals_size() > 0) {
                pv_deal_id << imp.pmp().deals(0).id();
            }
        }
    }
    if (pv_deal_id.str().length() > 0)
        PV_SET(deal_id, pv_deal_id.str());
    //orsrsp algo_pvlogs 打印
    for (int i = 0; i < orsrsp.algo_pvlogs_size(); i++) {
        ss << orsrsp.algo_pvlogs(i).key() << "="
                << orsrsp.algo_pvlogs(i).value() << "`";
    }
    if (orsrsp.algoed_ads_size() > 0)
        PV_SET(bid_ad_num, orsrsp.algoed_ads_size());

      
    if (bidreq.has_device()) {
        if (bidreq.device().has_ip()) {
            PV_SET(ip, bidreq.device().ip());
        }
        if (bidreq.device().has_user_agent()) {
            PV_SET(ua, bidreq.device().user_agent());
        }
        if (bidreq.device().has_os()) {
            PV_SET(os, bidreq.device().os());
        }
        if (bidreq.device().has_os_ver()) {
            PV_SET(os_ver, bidreq.device().os_ver());
        }
        if (bidreq.device().has_brand()) {
            PV_SET(brand, bidreq.device().brand());
        }
        if (bidreq.device().has_model()) {
            PV_SET(model, bidreq.device().model());
        }
        if (bidreq.device().has_id()) {
            PV_SET(dev_id, bidreq.device().id());
//            PV_SET(mac, bidreq.device().id());
//            PV_SET(idfa, bidreq.device().id());
        }
        if (bidreq.device().has_connection_type()) {
            //PV_SET(network, bidreq.device().connection_type());
            //转换为pvlong的connetion type
            switch (bidreq.device().connection_type()) {
            case rtb::CONNECTION_TYPE_WIFI:
                PV_SET(network, PV_LOG_CONN_TYPE_WIFI);
                break;
            case rtb::CONNECTION_TYPE_CELLULAR_DATA_2G:
                PV_SET(network, PV_LOG_CONN_TYPE_2G);
                break;
            case rtb::CONNECTION_TYPE_CELLULAR_DATA_3G:
                PV_SET(network, PV_LOG_CONN_TYPE_3G);
                break;
            case rtb::CONNECTION_TYPE_CELLULAR_DATA_4G:
                PV_SET(network, PV_LOG_CONN_TYPE_4G);
                break;
            default:
                PV_SET(network, PV_LOG_CONN_TYPE_UNKOWN);
                break;
            }
        }
        if (bidreq.device().has_geo()) {
            if (bidreq.device().geo().has_lat()) {
                PV_SET(latitude, bidreq.device().geo().lat());
            }
            if (bidreq.device().geo().has_lon()) {
                PV_SET(longitude, bidreq.device().geo().lon());
            }
        }
        if (bidreq.device().has_ext()
                && bidreq.device().ext().has_dev_resolution()) {
            PV_SET(dev_size, bidreq.device().ext().dev_resolution());
        }

    }
    if (bidreq.has_user()) {
      const rtb::User& user = bidreq.user();
        if (user.has_id()) {
            PV_SET(uid, user.id());
        }
        if (user.has_ext() && user.ext().has_aid()) {
            PV_SET(aid, user.ext().aid());
        }
        if(user.has_gender())
        {
          PV_SET(gender, user.gender());
        }
        if(user.has_year_of_birth())
        {
          PV_SET(yob, user.year_of_birth());
        }
        std::stringstream u_ss;
        for(int i=0;i<user.data_size();i++)
        {
          const rtb::Data& data=user.data(i);
          if(!data.has_id())
            continue;
          for(int j=0;j<data.segments_size();j++)
          {
            if(data.segments(j).has_id())
            {
              u_ss<<data.id()<<"_"<<data.segments(j).id()<<"|";
            }
          }
        }
        if(u_ss.str().length()>0)
        {
          PV_SET(udata, u_ss.str());
        }
    }
    PV_SET(sid, bidreq.id());
    PV_SET(bid, bidreq.id());
    PV_SET(pv_type, 1);            //到control的都是正常流量

    if (bidreq.has_site()) {
        if (bidreq.site().site_categories_size() > 0) {
            std::stringstream s1;
            for (int i = 0; i < bidreq.site().site_categories_size(); i++) {
                s1 << bidreq.site().site_categories(i) << "|";
            }
            PV_SET(web_cate, s1.str());
        }
        if (bidreq.site().has_page()) {
            PV_SET(url, bidreq.site().page());
        }
        if (bidreq.site().has_ref()) {
            PV_SET(referer, bidreq.site().ref());
        }
    }
//    TODOFIX(PV_SET(org_category,), "不清楚怎么取");
    if (bidreq.impressions_size() > 0) {
        //只考虑一个广告位的情况
        rtb::Impression impr = bidreq.impressions(0);
        PV_SET(pid, impr.id());
        if (impr.has_ext()) {
            const rtb::Impression_Ext& imprExt = impr.ext();
            if (imprExt.has_view_screen()) {
                PV_SET(view_screen, imprExt.view_screen());
            }

            if (imprExt.has_ad_num()) {
                PV_SET(req_ad_num, imprExt.ad_num());
            }

        }

        std::stringstream vt_ss;
        if (impr.view_types_size() > 0) {
            for (int i = 0; i < impr.view_types_size(); i++) {
                vt_ss << impr.view_types(i) << "|";
            }
        } else {
            if (impr.has_ext() && impr.ext().has_view_type()) {
                vt_ss << impr.ext().view_type() << "|";
            }
        }
        if (vt_ss.str().length() > 0) {
            PV_SET(view_type, vt_ss.str());
        }

        if (impr.has_banner()) {
            if (impr.banner().has_width() && impr.banner().has_height()) {
                std::stringstream s1;
                s1 << impr.banner().width() << "*" << impr.banner().height();
                PV_SET(size, s1.str());
            }
        } else if (impr.has_video()) {
            if (impr.video().has_width() && impr.video().has_height()) {
                std::stringstream s1;
                s1 << impr.video().width() << "*" << impr.video().height();
                PV_SET(size, s1.str());
            }
        }

        if (impr.has_bidfloor()) {
            PV_SET(min_cpm_price, impr.bidfloor());
        }

        if (impr.has_video()) {
            if (impr.video().has_ext() && impr.video().ext().has_content()) {
                if (impr.video().ext().content().has_title())
                    PV_SET(title, impr.video().ext().content().title());
                int keywords_size =
                        impr.video().ext().content().keywords_size();
                if (keywords_size > 0) {
                    std::stringstream s1;
                    for (int i = 0; i < keywords_size; i++) {
                        s1 << impr.video().ext().content().keywords(i) << "|";
                    }
                    if (s1.str().length() > 0)
                        PV_SET(keywords, s1.str());
                }
            }
            if (impr.video().has_max_duration()) {
                PV_SET(duration, impr.video().max_duration());
            }

            const rtb::Content * content = NULL;
            if (impr.video().has_ext() && impr.video().ext().has_content()) {
                content = &(impr.video().ext().content());
            }
            if (content != NULL) {
                const rtb::Content::Ext & con_ext = content->ext();
                int direct_size = con_ext.direct_size();
                for (int i = 0; i < direct_size; i++) {
                    const rtb::Content::Ext::Direct & direct = con_ext.direct(
                            i);
                    if (direct.key() == "channel") {
                        PV_SET(cnl, direct.value());
                    } else if (direct.key() == "cs") {
                        PV_SET(cnl2, direct.value());
                    } else if (direct.key() == "usr") {
                        PV_SET(video_owner, direct.value());
                    } else if (direct.key() == "s") {
                        PV_SET(show_id, direct.value());
                    } else if (direct.key() == "vid") {
                        PV_SET(vid, direct.value());
                    }

                }
            }

        }
    }
    PV_SET(is_prefer, 1);            //暂时不支持优先交易
    if (bidreq.has_ext() && bidreq.ext().has_page_pv_id()) {
        PV_SET(page_sid, bidreq.ext().page_pv_id());
    }
    if (sessdata.prog & PROG_ADAPTER_RSP) {
        //广告:
        if (bidrsp.bid_seats_size() > 0) {
            const rtb::BidSeat & bidseat = bidrsp.bid_seats(0);

            if (bidseat.bids_size() > 0) {
                //打印多个PV值
                std::stringstream pv_creative_id;
                std::stringstream pv_ad_id;
                std::stringstream pv_bid_price;
                std::stringstream pv_cost_price;
                std::stringstream pv_billing_type;
                std::stringstream pv_org_price;
                std::stringstream pv_creative_material_id;
                std::stringstream pv_campaign_id;
                for (int bid_pos = 0; bid_pos < bidseat.bids_size();
                        bid_pos++) {
                    const rtb::Bid & tmp_bid = bidseat.bids(bid_pos);
                    if (tmp_bid.has_creative_id())
                        pv_creative_id << tmp_bid.creative_id() << "|";
                    if (tmp_bid.has_ext()) {
                        const rtb::Bid::Ext & bidext = tmp_bid.ext();
                        if (bidext.has_adgroup_id())
                            pv_ad_id << bidext.adgroup_id() << "|";
                        if (bidext.has_cost_price())
                            pv_cost_price << bidext.cost_price() << "|";
                        if (bidext.has_billing_type())
                            pv_billing_type << bidext.billing_type() << "|";
                        if (bidext.has_org_price())
                            pv_org_price << bidext.org_price() << "|";
                    }
                    if (tmp_bid.has_price())
                        pv_bid_price << tmp_bid.price() << "|";
                    if (tmp_bid.has_material_id())
                        pv_creative_material_id << tmp_bid.material_id() << "|";
                    if (tmp_bid.has_campaign_id())
                        pv_campaign_id << tmp_bid.campaign_id() << "|";
                }
                if (pv_creative_id.str().length() > 0)
                    PV_SET(creative_id, pv_creative_id.str());
                if (pv_ad_id.str().length() > 0)
                    PV_SET(ad_id, pv_ad_id.str());
                if (pv_bid_price.str().length() > 0)
                    PV_SET(bid_price, pv_bid_price.str());
                if (pv_cost_price.str().length() > 0)
                    PV_SET(cost_price, pv_cost_price.str());
                if (pv_billing_type.str().length() > 0)
                    PV_SET(billing_type, pv_billing_type.str());
                if (pv_org_price.str().length() > 0)
                    PV_SET(org_price, pv_org_price.str());
                if (pv_creative_material_id.str().length() > 0)
                    PV_SET(creative_material_id, pv_creative_material_id.str());
                if (pv_campaign_id.str().length() > 0)
                    PV_SET(campaign_id, pv_campaign_id.str());

                const rtb::Bid & bid = bidseat.bids(0);
                if (bid.has_impid()) {
                    PV_SET(imp_id, bid.impid());
                }
                PV_SET(prod_type, 1);

            }

        }
    }
    //无线流量
    if (bidreq.has_app()) {
        if (bidreq.app().has_bundle()) {
            PV_SET(pkg_name, bidreq.app().bundle());
        }

        if (bidreq.app().app_categories_size() > 0) {
            std::stringstream s1;
            for (int i = 0; i < bidreq.app().app_categories_size(); i++) {
                s1 << bidreq.app().app_categories(i) << "|";
            }
            PV_SET(app_cate, s1.str());
        }
        if (bidreq.app().page_categories_size() > 0) {
            std::stringstream s1;
            for (int i = 0; i < bidreq.app().page_categories_size(); i++) {
                s1 << bidreq.app().page_categories(i) << "|";
            }
            PV_SET(cont_cate, s1.str());
        }
        if (bidreq.app().has_ext()) {
            size_t ntemp_cnt = bidreq.app().ext().native_template_ids_size();
            std::stringstream s1;
            for (size_t i = 0; i < ntemp_cnt; i++) {
                s1 << bidreq.app().ext().native_template_ids(i) << "|";
            }
            PV_SET(ntemp_id, s1.str());

            //打印PV日志，信息流屏幕参数
            if (bidreq.app().ext().native_ad_template_size() > 0) {
                std::stringstream pv_wh_s;
                for (int i = 0;
                        i < bidreq.app().ext().native_ad_template_size(); i++) {
                    pv_wh_s << bidreq.app().ext().native_ad_template(i).w()
                            << "*"
                            << bidreq.app().ext().native_ad_template(i).h()
                            << "|";
                }

                PV_SET(ntemp_size, pv_wh_s.str());
            }
        }

    }

    for (int i = 0; i < snrsp.scoring_pvlogs_size(); i++) {
        ss << snrsp.scoring_pvlogs(i).key() << "=" << snrsp.scoring_pvlogs(i).value() << "`";
    }

    if (sessdata.prog & PROG_QP_RSP) {
        PV_SET(qp_flag, 1);
        PV_SET(qp_latency, (sessdata.time_qprsp - sessdata.time_qpreq));
        status_qp_latency(sessdata.time_qprsp - sessdata.time_qpreq);
        if (qprsp.has_hostname()) {
            PV_SET(qp_hostname, qprsp.hostname());
        }
    }
    if (sessdata.prog & PROG_SN_RSP) {
        if (snrsp.ads_size() > 0) {
            const sn::Ads & ads = snrsp.ads(0);
            PV_SET(sn_flag, ads.ad_size());
            if (snrsp.has_hostname()) {
                PV_SET(sn_hostname, snrsp.hostname());
            }
            size_t sn_ad_cnt = ads.ad_size();
            std::stringstream s1;
            for (size_t i = 0; i < sn_ad_cnt; i++) {
                s1 << ads.ad(i).adgroup_id() << "|";
            }
            PV_SET(sn_ad_list, s1.str());
        }
        PV_SET(sn_latency, (sessdata.time_snrsp - sessdata.time_snreq));
        status_sn_latency(sessdata.time_snrsp - sessdata.time_snreq);
    }
    if (sessdata.prog & PROG_FB_RSP) {
        PV_SET(fb_flag, filterrsp.ad_size());
        PV_SET(fb_latency, (sessdata.time_fbrsp - sessdata.time_fbreq));
        status_fb_latency(sessdata.time_fbrsp - sessdata.time_fbreq);
    }
    if (sessdata.prog & PROG_DN_RSP) {
        PV_SET(dn_flag, dnrsp.creative_size());
        PV_SET(dn_latency, (sessdata.time_dnrsp - sessdata.time_dnreq));
        status_dn_latency(sessdata.time_dnrsp - sessdata.time_dnreq);
        if (dnrsp.has_hostname()) {
            PV_SET(dn_hostname, dnrsp.hostname());
        }
    }
    if (sessdata.prog & PROG_ORS_RSP) {
        PV_SET(ors_flag, 1);
        PV_SET(ors_latency, (sessdata.time_orsrsp - sessdata.time_orsreq));
        status_ors_latency(sessdata.time_orsrsp - sessdata.time_orsreq);
        if (orsrsp.has_hostname()) {
            PV_SET(ors_hostname, orsrsp.hostname());
        }
    }
    if (sessdata.prog & PROG_ADAPTER_RSP) {
        PV_SET(bid_flag, 1);
    }
    PV_SET(bid_latency, (sessdata.time_bidrsp - sessdata.time_bidreq));

    uint64_t bid_latency = (sessdata.time_bidrsp - sessdata.time_bidreq);

    status_bid_latency(bid_latency);

    ss << std::endl;
    LOG_DEBUG("PVLOG[%s]", ss.str().c_str());
    if (sessdata.traffic_source == 0) {
        LOG_ERROR("error pvlog:%s", ss.str().c_str());
    }
    if (sessdata.traffic_source > 8 || sessdata.traffic_source < 0) {
        LOG_ERROR("fatal pvlog:%s", ss.str().c_str());
    } else {
        PvlogTrans::get_mutable_instance().trans(ss.str().c_str(),
                ss.str().length());
    }

#undef PV_SET

}

}
}
