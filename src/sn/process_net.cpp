/**
 **/
#include "process_net.h"
#include "myindex.h"
#include "util/log.h"
#include "util/func.h"
#include "util/monitor.h"
#include "util/util_str.h"
#include "monitor_api.h"
#include "sn_attr.h"
#include "protocol/src/poseidon_proto.h"
#include <unistd.h>
#include <algorithm>


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

namespace poseidon{
namespace sn{

int ProcessNet::rand_sort(const std::set<IndexAd> & setad, std::vector<IndexAd> & vrad)
{
    int rt=0;
    do{
        std::set<IndexAd>::const_iterator it;
        for(it=setad.begin(); it != setad.end(); it++)
        {
            vrad.push_back(*it);
        }
        if(vrad.size() > MAX_RETURN_SIZE )
        {
            std::random_shuffle(vrad.begin(), vrad.end());
        }

    }while(0);
    return rt;
}

int ProcessNet::handle_read(const char * buf, const int len, struct sockaddr_in & client_addr)
{
    int rt=0;
    SessData * sess=NULL;
    do{
        MON_ADD(ATTR_SN_REQ, 1);
        sess = new(std::nothrow) SessData();
        if(sess == NULL)
        {
            break;
        }

        util::Func::get_time_ms(sess->time_sn_req);

        sn::SNRequest & req=sess->req ;
        sn::SNResponse & rsp=sess->rsp;

        if( !req.ParseFromArray(buf, len) )
        {
            LOG_ERROR("req ParseFromArray error");
            MON_ADD(ATTR_REQ_PARSE_ERR, 1);
            rt = -1;
            break;
        }
        LOG_DEBUG("recv sn req[%s]\n", req.DebugString().c_str());
        int adz_size=req.adz_info_size();

        if(adz_size>0)
        {
            const sn::AdzInfo & adz=req.adz_info(0);
            Query query;
            rt=build_query(req, query, sess);
            if(rt != 0)
            {
                break;
            }
            std::set<IndexAd> setad;
            util::Func::get_time_ms(sess->time_sn_query_start);
            rt=MyIndex::get_mutable_instance().query(query, setad);
            util::Func::get_time_ms(sess->time_sn_query_end);
            if(rt != 0)
            {
                LOG_ERROR("ProcessNet query error[%d]\n", rt);
                break;
            }
            std::vector<IndexAd> vr_ad;
            std::set<IndexAd>::const_iterator it;
            for(it=setad.begin(); it != setad.end(); it++)
            {
                vr_ad.push_back(*it);
            }

            //TODO:把setad进行随机排序
            if(vr_ad.size() >= MAX_RETURN_SIZE )
            {
                std::random_shuffle(vr_ad.begin(), vr_ad.end());
            }
            
            LOG_DEBUG("query get ad_cnt[%d]", setad.size() );
            poseidon::sn::Ads * pAds=rsp.add_ads();
            //step 2:get adinfo from adid
#if 0
            int ad_cnt;
            if(setad.size() > MAX_RETURN_SIZE)
            {
                ad_cnt=MAX_RETURN_SIZE;
            }else
            {
                ad_cnt=setad.size();
            }
#endif
            int adidx=0;

            std::vector<IndexAd>::iterator itvr;
            for(itvr=vr_ad.begin();
                    itvr!= vr_ad.end(); itvr++)
            {
                //过滤：
                bool allow=filter_ad(sess, adz, *itvr); 
                if(!allow)
                {
                    continue;
                }
                poseidon::common::Ad * pad=pAds->add_ad();
                //计划组id
//                uint32 campaign_id = 1;
                pad->set_campaign_id(itvr->campaign_id);

                //独立访客展现量控制
//                optional int64 freq_impression = 2;
                pad->set_freq_impression(itvr->freq_impression);
   
                //每日预算, -1, 表示不限
//                optional int64 campaign_daily_budget = 3;
                if(itvr->campaign_daily_budget > 0)
                {
                    pad->set_campaign_daily_budget(itvr->campaign_daily_budget);
                }

                //投放策略
//                optional poseidon.rtb.SendSpeedType send_speed = 4;

                if(itvr->send_speed == rtb::SST_FAST)
                {
                    pad->set_send_speed(rtb::SST_FAST);
                }else
                {
                    pad->set_send_speed(rtb::SST_SMOOTH);
                }

                //竞价类型： T:CPT, M:CPM  C:CPC  
//                optional string billing_type = 5;
                pad->set_billing_type(itvr->billing_type); 

                //计划投递时段, counter拿到这个字段要做预算的计算和平滑
                //optional uint32 post_hours = 6;
                //
//TODO: 根据post_hours做过滤
                if(itvr->post_hours >0)
                {
                    pad->set_post_hours(itvr->post_hours);
                }
                if(itvr->campaign_type > 0)
                {
                    pad->set_campaign_type(itvr->campaign_type);
                }
   
                //广告主id
//                optional uint32 advertiser_id = 7;
                if(itvr->advertiser_id > 0)
                {
                    pad->set_advertiser_id(itvr->advertiser_id);
                }
   
                //广告主预算
//                optional uint32 advertiser_budget = 8;
                if(itvr->advertiser_budget > 0)
                {
                    pad->set_advertiser_budget(itvr->advertiser_budget); 
                }

                //推广单元id
//                optional uint32 adgroup_id = 9;
                if(itvr->adgroup_id != 0)
                {
                    pad->set_adgroup_id(itvr->adgroup_id);
                }
   
                //创意id, 经过了templateid, category, level, format等过滤之后的
//                optional uint64 creative_id = 10;
                if(itvr->creative_id != 0)
                {
                    pad->set_creative_id(itvr->creative_id);
                }

                //广告主原始报价
//                optional uint32 org_price = 11;
                if(itvr->org_price != 0 )
                {
                    pad->set_org_price(itvr->org_price);
                }

                //算法报价
//                optional uint32 algo_price = 12;
                if(itvr->algo_price != 0)
                {
                    pad->set_algo_price(itvr->algo_price);
                }
   
                //索引离线海选分数
//                optional uint32 score = 13;
                if(itvr->score != 0)
                {
                    pad->set_score(itvr->score);
                }
   
                //创意类型 1. 图片 2 文字 3 flash 4 video等 
//                optional uint32 creative_format = 14;
                if(itvr->creative_format != 0)
                {
                    pad->set_creative_format(itvr->creative_format);
                }

                //广告主想要拿下流量的基础出价，算法报价在 org_price(最高报价)之下，在base_price左右浮动
//                optional uint32 base_price = 15;
//木有这个值
//                pad->set_base_price(itvr->base_price);

                //报价方式 1: 智能出价; 2: 固定报价; 
//                optional uint32 bid_type = 16;
                if(itvr->bid_type != 0)
                {
                    pad->set_bid_type(itvr->bid_type);
                }

                if(itvr->product != 0)
                {
                    pad->set_product(itvr->product);
                }

                if(itvr->inner_advertiser_id != 0)
                {
                    pad->set_inner_advertiser_id(itvr->inner_advertiser_id);
                }
                if(!itvr->ch.empty())
                {
                    pad->set_ch(itvr->ch);
                }
                if(itvr->gid != 0)
                {
                    pad->set_gid(itvr->gid);
                }
                
                if(itvr->premium_rate != INT_UNINIT)
                {
                    pad->set_premium_rate(itvr->premium_rate);
                }

                if(!(itvr->advertiser_balance_day.empty()))
                {
                    pad->set_advertiser_balance_day(itvr->advertiser_balance_day);
                }
                if(itvr->advertiser_balance != INT_UNINIT)
                {
                    pad->set_advertiser_balance(itvr->advertiser_balance);
                }

                if(!itvr->deal_id.empty())
                {
                    common::Ad_PdbData * pdb_data=pad->mutable_pdb_data();
                    pdb_data->set_deal_id(itvr->deal_id);
                    if(itvr->settle_price > 0)
                    {
                        pdb_data->set_settle_price(itvr->settle_price);
                    }
                    if(itvr->total_exp > 0)
                    {
                        pdb_data->set_total_exp(itvr->total_exp);
                    }
                    if(itvr->day_exp > 0)
                    {
                        pdb_data->set_day_exp(itvr->day_exp);
                    }
                    if(itvr->campaign_quota > 0)
                    {
                        pdb_data->set_campaign_quota(itvr->campaign_quota);
                    }
                    if(itvr->fill_rate > 0)
                    {
                        pdb_data->set_fill_rate(itvr->fill_rate);
                    }
                }

                adidx++;
                if(adidx>=MAX_RETURN_SIZE)
                {
                    break;
                }
            }
            pAds->set_err_code(common::ERROR_NONE);
            LOG_DEBUG("return ad cnt[%d]\n", adidx);
        }
        
        rsp.set_err_code(common::ERROR_NONE);
        rsp.set_session_id(req.session_id());

        if(rsp.ads_size()>0)
        {
            if(rsp.ads(0).ad_size()==0)
            {
                MON_ADD(ATTR_SN_RETURN_ZERO, 1);
            }else
            {
                MON_ADD(ATTR_SN_RETURN_NOT_ZERO, 1);
            }
            if(rsp.ads(0).ad_size()==MAX_RETURN_SIZE)
            {
                MON_ADD(ATTR_SN_RETURN_MAX, 1);
            }
        }
        char hostname[256];
        memset(hostname, 0x00, 256); 
        gethostname(hostname, 256);
        util::UtilStr ustr;
        ustr.format("%s_%d", hostname, getpid());

        rsp.set_hostname(ustr.str());

        if(req.has_trace_id())
        {
            rsp.set_trace_id(req.trace_id());
        }

        std::string rspstr;
        if(!rsp.SerializeToString(&rspstr))
        {
            LOG_ERROR("rsp.SerializeToString error");
            rt=-1;
            break;
        }
        LOG_DEBUG("snrsp[%s], rsp.ByteSize[%u], rspstr.length[%u]", rsp.DebugString().c_str(), rsp.ByteSize(), rspstr.length());
        
        send_pkg(rspstr.c_str(), rspstr.length(), client_addr);
        util::Func::get_time_ms(sess->time_sn_rsp);
        MON_ADD(ATTR_SN_RSP, 1);
        status_latency(sess->time_sn_rsp-sess->time_sn_req);

    }while(0);
    if(sess != NULL)
    {
        delete sess;
    }
    return rt;
}

void ProcessNet::status_latency(uint64_t sn_latency)
{
    if(sn_latency < 5)
    {
        MON_ADD(ATTR_SN_LATENCY_LESS_5, 1);
    }else if(sn_latency < 10)
    {
        MON_ADD(ATTR_SN_LATENCY_LESS_10, 1);
    }else if(sn_latency < 20)
    {
        MON_ADD(ATTR_SN_LATENCY_LESS_20, 1);
    }else
    {
        MON_ADD(ATTR_SN_LATENCY_GT_20, 1);
    }
}


int ProcessNet::build_query(const sn::SNRequest & snreq, Query & query, SessData *sess )
{
    int rt=0;
    do{
        const sn::AdzInfo & adz=snreq.adz_info(0);
        int tar_size=adz.targetting_size();
        int idx=0;
        sess->user_city_id=0;//默认0，表示没有该用户城市信息
        for(idx=0; idx<tar_size; idx++)
        {
            const common::Targetting & tar=adz.targetting(idx);
            int type=tar.type();
            if(tar.value_size() == 0)
            {
                continue;
            }
            std::string value=tar.value(0);
            int intvalue=atoi(value.c_str());
            //用户所在地不作为检索条件
            if(type == CITY_TARG_ID)
            {
                sess->user_city_id=intvalue;
                continue;
            }
            TagType tvalue=(int64_t(type)<<32)|intvalue;
            query.push_back(tvalue);
        }
        if(adz.has_view_type())
        {
            TagType tvalue=(int64_t(VIEW_TYPE_TARG_ID)<<32)|adz.view_type();
            query.push_back(tvalue);
        }
        if(snreq.has_traffic_source())
        {
            TagType tvalue=(int64_t(SOURCE_TARG_ID)<<32)|snreq.traffic_source();
            query.push_back(tvalue);
        }
        if(adz.has_deal_id())
        {
            TagType tvalue=(int64_t(DEAL_ID_TARG_ID)<<32)|util::Func::to_int(adz.deal_id());
            query.push_back(tvalue);
        }else
        {
            TagType tvalue=(int64_t(DEAL_ID_TARG_ID)<<32)|0;
            query.push_back(tvalue);
        }

    }while(0);
    return rt;
}

/**
 * @brief               过滤广告
 * @param sess          [IN], Session
 * @param adz           [IN],广告位信息
 * @param ad            [IN],待过滤的广告
 * @return              允许广告返回true, 否则返回false
 **/
bool ProcessNet::filter_ad(const SessData * sess, const sn::AdzInfo & adz, const IndexAd & ad)
{
    bool rt=true;
    do{
#if 0
        const SNRequest & snreq=sess->req;

        if( (snreq.traffic_source() != ad.source) ||
             ad.view_types.count(adz.view_type()) == 0 )
        {
            rt=false;
            break;
        }
#endif

        //是否是投放时段
        if(ad.ad_time_type==POST_TYPE_HOUR)
        {
            struct tm tminfo;
            util::Func::get_time_info(tminfo);

            /*第nseg个时段, 半个小时一个时段*/
            int nseg=tminfo.tm_hour*2+(tminfo.tm_min>=30?1:0);
           
            int64_t flag= ad.post_hours &(((int64_t)1)<<nseg);
            if(!flag)
            {
                LOG_DEBUG("current seg[%d]post_hours[%llu], filter\n", nseg, ad.post_hours);
                rt=false;
                break;
            }
        }

#if 0
        /*广告允许的创意格式*/
        size_t format_size=adz.creative_format_size();
        //没有creative_format认为所有格式都允许
        if(format_size > 0)
        {
            rt=false;
            for(size_t i=0; i<format_size; i++)
            {
                if(ad.creative_format == (uint32_t)adz.creative_format(i))
                {
                    rt=true;
                    break;
                }
            }
            if(!rt)
            {
                LOG_DEBUG("ad.creative_format[%u] filter", ad.creative_format);
                break;
            }
        }

        int bformat_size=adz.blocked_creative_format_size();
        for(int i=0; i<bformat_size; i++ )
        {
            if(ad.creative_format == (uint32_t)adz.blocked_creative_format(i) )
            {
                LOG_DEBUG("ad.creative_format[%u] be blocked", ad.creative_format);
                rt=false;
                break;
            }
        }
        if(!rt)
        {
            LOG_DEBUG("ad.creative_format[%u] filter, by blocked creative_format", ad.creative_format);
            break;
        }
#endif


        //所在城市过滤, 广告没有城市定向，则放过
        int city_size=ad.city_id.size();
        if(city_size>0)
        {
            rt=false;
            if(sess->user_city_id == 0)
            {
                //用户没有城市定向数据，广告有，则过滤
                break;
            }
            int user_prov=sess->user_city_id/10000;
            int user_city=(sess->user_city_id-user_prov*10000)/100;
            int user_area=sess->user_city_id-user_prov*10000-user_city*100;
            for(int i=0; i<city_size; i++)
            {
                int ad_city=ad.city_id[i];
                //判断用户所在城市是否在广告的定向城市列表
                if(ad_city/10000 != user_prov)
                {
                    continue;
                }
                ad_city=ad_city%10000;
                if(ad_city == 0)
                {
                    rt=true;
                    break;
                }
                if(ad_city/100 != user_city)
                {
                    continue;
                }
                ad_city=ad_city%100;
                if(ad_city == 0)
                {
                    rt=true;
                    break;
                }
                if(ad_city != user_area)
                {
                    continue;
                }else
                {
                    rt=true;
                    break;
                }

            }
        }
        if(!rt)
        {
            break;
        }


    }while(0);
    return rt;
}


}
}


