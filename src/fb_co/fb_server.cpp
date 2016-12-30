/*
 * fb_server.cpp
 * Created on: 2016-12-16
 */

#include "fb_co/fb_server.h"

#include <sstream>

#include "monitor_api.h"
#include "util/log.h"
#include "util/func.h"
#include "protocol/src/poseidon_proto.h"

#include "fb_co/attr.h"
#include "fb_co/config.h"
#include "fb_co/session.h"

namespace poseidon {
namespace feedback {

const std::string MEM_FREQUENCY            = "MEM_FREQUENCY:";
const std::string GAMES_AD_COST_ADVER      = "GAMES_AD_COST_ADVER:";     // 广告主每日花费
const std::string GAMES_AD_COST_CAMP       = "GAMES_AD_COST_CAMP:";      // 广告(推广)计划每日金额花费
const std::string GAMES_AD_COST_AD         = "GAMES_AD_COST_AD:";        // 广告组每日金额花费
const std::string GAMES_AD_COST_CREATIVE   = "GAMES_AD_COST_CREATIVE:";  // 创意每日金额花费
const std::string MEM_KEY_DEAL_ID          = "DEAL_ID:";                 // 曝光次数
const std::string MEM_KEY_ADVER            = "ADVER:";                   // 广告主
const std::string MEM_KEY_CAMP             = "CAMP:";                    // 广告(推广)计划
const std::string MEM_KEY_AD               = "AD:";                      // 广告
const std::string MEM_KEY_CREATIVE         = "CREATIVE:";                // 创意
const std::string MEM_KEY_DEAL_CAMPAIGN_ID = "DEAL_CAMPAIGN_ID:";        // 曝光的创意ID
const std::string MEM_STATISTICS_COM_DEF   = ":";                        // ...

using poseidon::util::Log;
using poseidon::util::Func;

FBServer::FBServer(void) : timer_(NULL)
{

}

FBServer::~FBServer(void)
{
    if (timer_) {
        timer_->Stop();
    }
}

int FBServer::Init(void)
{
    /* 100 millisecond timer */
    timer_ = co2::timer::Timer::Create(&FBServer::OnTimer, *this, 100);
    if (!timer_) {
        LOG_ERROR("Create timer failed!");
        return -1;
    }

    if (0 != pool_.Init()) {
        LOG_ERROR("RedisPool Init failed!");
        return -1;
    }

    timer_->Start();
    return 0;
}

void FBServer::OnTimer(void *arg)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    int64_t now = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    advertiser_cache_.OnTimer(now);
    campaign_cache_.OnTimer(now);
    group_cache_.OnTimer(now);
    creative_cache_.OnTimer(now);
    count_cache_.OnTimer(now);
    deal_cache_.OnTimer(now);
    deal_campaign_cache_.OnTimer(now);
}

int FBServer::OnDatagram(const std::string &host, int port, const char *buf,
                         int size)
{
    MON_ADD(ATTR_FB_REQ, 1);

    co2::redis::Redis *redis = NULL;

    std::list<redisReply *> replies;

    int rt = -1;
    for (int retry = 0; retry < 3; retry++) {
        redis = pool_.GetConnection();
        if (redis)
            break;
    }
    if (!redis) {
        /* TODO: redis alarm here ... */
        return rt;
    }

    poseidon::feedback::SessData sess;
    sess.host = host;
    sess.port = port;

    do {
        FeedbackRequest &fbreq = sess.fbreq;
        if (!fbreq.ParseFromArray(buf, size)) {
            MON_ADD(ATTR_FB_PARSE_ERR, 1);
            LOG_ERROR("new SessData return NULL");
            rt = -1;
            break;
        }

        LOG_DEBUG("fbreq[%s]", fbreq.DebugString().c_str());

        /* TODO: to define user's deviceID? */
        std::string uid = sess.fbreq.dev_id();
        if (uid.empty() || uid.length() <= 4) {
            /* what is acookie */
            uid = fbreq.acookie();
        }

        /* current time */
        std::string datetime;
        Func::get_time_str(&datetime, "%Y%m%d");

        LOG_DEBUG("time = %s", datetime.c_str());

        /* current time - 1day */
        char last_day[16];
        time_t now = time(0);
        struct tm *ts = localtime(&now);
        ts->tm_mday--;
        mktime(ts);
        strftime(last_day, sizeof(last_day), "%Y%m%d", ts);
        sess.last_day = last_day;

        std::string adNumCmd     = "hgetall " + MEM_FREQUENCY + uid;
        std::string lastdayCmd   = "hmget "   + GAMES_AD_COST_ADVER + last_day;
        std::string adverCmd     = "hmget "   + GAMES_AD_COST_ADVER + datetime.c_str();
        std::string campaignCmd  = "hmget "   + GAMES_AD_COST_CAMP + datetime.c_str();
        std::string adgroupCmd   = "hmget "   + GAMES_AD_COST_AD + datetime.c_str();
        std::string creativeCmd  = "hmget "   + GAMES_AD_COST_CREATIVE + datetime.c_str();
        std::string dealCmd      = "hmget "   + MEM_KEY_DEAL_ID + datetime.c_str();
        std::string dealCampaign = "hmget "   + MEM_KEY_DEAL_CAMPAIGN_ID + datetime.c_str();

        std::stringstream ss; // for the log

        // 1. 遍歷这个终端查看广告集合
        for (int i = 0; i < fbreq.ad_size(); i++) {

            // 2. 获取所有的广告主内存快照
            uint32_t advertiser_id = fbreq.ad(i).inner_advertiser_id();
            if (advertiser_id != 0) {
                ReqAdvertiser(advertiser_id, sess.last_day, adverCmd, lastdayCmd, &sess);
                ss << "advertiser_id: " << advertiser_id << ",";
            }

            // 3. 获取所有的推广计划内存快照
            uint32_t campaign_id = fbreq.ad(i).campaign_id();
            if (campaign_id != 0) {
                ReqAdvertiseCampaign(campaign_id, campaignCmd, &sess);
                ss << "campaign_id: " << campaign_id << ",";
            }

            // 4. 获取所有的广告组内存快照, 不同广告主的推广计划ID和广告ID是不同的
            uint32_t adgroup_id = fbreq.ad(i).adgroup_id();
            if (adgroup_id != 0) {
                ReqAdvertiseGroup(adgroup_id, adgroupCmd, &sess);
                ss << "adgroup_id: " << adgroup_id << ",";
            }

            // 5.创意ID
            uint64_t creative_id = fbreq.ad(i).creative_id();
            if (creative_id != 0) {
                ReqAdvertiseCreative(creative_id, creativeCmd, &sess);
                ss << "creative_id: " << creative_id << "," ;
            }

            // PDB
            if (fbreq.ad(i).has_pdb_data()) {
                const std::string &deal_id = fbreq.ad(i).pdb_data().deal_id();
                ReqPdbDeal(deal_id, campaign_id, dealCmd, dealCampaign, &sess);
                ss << "deal_id: " << deal_id;
            } // end if pdb_data
            ss << "|";
        } // end for

        LOG_DEBUG("%s", ss.str().c_str());

        // begin query redis...
        LOG_DEBUG("adNumCmd[%s]", adNumCmd.c_str());
        redis->AppendCommand(adNumCmd.c_str());

        // 广告主
        LOG_DEBUG("advertiserCmd[%s]", adverCmd.c_str());
        if (sess.advertiserVt.size()) {
            redis->AppendCommand(adverCmd.c_str());
        }

        // 推广计划
        LOG_DEBUG("campaignCmd[%s]", campaignCmd.c_str());
        if (sess.campaignVt.size()) {
            redis->AppendCommand(campaignCmd.c_str());
        }

        // 广告组
        LOG_DEBUG("adgroupCmd[%s]", adgroupCmd.c_str());
        if (sess.adGroupVt.size()) {
            redis->AppendCommand(adgroupCmd.c_str());
        }

        // 创意
        if (sess.creativeVt.size()) {
            LOG_DEBUG("creativeCmd[%s]", creativeCmd.c_str());
            redis->AppendCommand(creativeCmd.c_str());
        }

        // 曝光
        LOG_DEBUG("dealCmd[%s]", dealCmd.c_str());
        if (sess.dealVt.size()) {
            redis->AppendCommand(dealCmd.c_str());
        }

        // PDB交易
        LOG_DEBUG("deal_campaign[%s]", dealCampaign.c_str());
        if (sess.dealCampaignVt.size()) {
            redis->AppendCommand(dealCampaign.c_str());
        }

        LOG_DEBUG("yesterday_CostVt[%s]", lastdayCmd.c_str());
        if (sess.lastdayAdvertiserVt.size()) {
            redis->AppendCommand(lastdayCmd.c_str());
        }

        replies = redis->Execute();
        // adNum
        auto reply_it = replies.begin();

        if (0 != OnAdvertiseNum(*reply_it, &sess))
            break;
        else
            ++reply_it;

        if (sess.advertiserVt.size()) {
            if (0 != OnAdvertiser(*reply_it, &sess))
                break;
            else
                ++reply_it;
        }

        if (sess.campaignVt.size()) {
            if (0 != OnAdvertiseCampaign(*reply_it, &sess))
                break;
            else
                ++reply_it;
        }

        if (sess.adGroupVt.size()) {
            if (0 != OnAdvertiseGroup(*reply_it, &sess))
                break;
            else
                ++reply_it;
        }

        if (sess.creativeVt.size()) {
            if (0 != OnAdvertiseCreative(*reply_it, &sess))
                break;
            else
                ++reply_it;
        }

        if (sess.dealVt.size()) {
            if (0 != OnPdbDeal(*reply_it, &sess))
                break;
            else
                ++reply_it;
        }

        if (sess.dealCampaignVt.size()) {
            if (0 != OnPdbDealCampaign(*reply_it, &sess))
                break;
            else
                ++reply_it;
        }

        if (sess.lastdayAdvertiserVt.size()) {
            if (0 != OnLastDayAdvertiser(sess.last_day, *reply_it, &sess))
                break;
            else
                ++reply_it;
        }
        rt = 0;

    } while (0);

    if (redis)
        pool_.Release(redis);

    if (replies.size()) {
        std::for_each(replies.begin(), replies.end(), freeReplyObject);
    }

    if (rt != 0)
        sess.fbrsp.set_error_code(common::ERROR_NO_RESULT);

    Response(&sess);
    return rt;
}

static int CheckReply(redisReply *reply)
{
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        LOG_WARN("OnAdvertiseNum error: %s", reply ? reply->str : "NoReply");
        return -1;
    }
    return 0;
}

static int GetMap(redisReply *reply, std::vector<std::string> &k,
                  std::map<std::string, std::string> &m,
                  Cache<std::string, std::string> &cache)
{
    if (reply->type != REDIS_REPLY_ARRAY) {
        LOG_WARN("reply is not array");
        return -1;
    }

    for (size_t i = 0; i < reply->elements; i++) {
        int type = reply->element[i]->type;
        LOG_DEBUG("element[%d] type = %d", i, type);

        if (type == REDIS_REPLY_STRING) {
            m[k[i]] = reply->element[i]->str;
            cache.Set(k[i], reply->element[i]->str);
        } else {
            m[k[i]] = "";
            cache.Set(k[i], "");
        }
    }
    return 0;
}

static int GetMap(redisReply *reply,
    std::map<std::string, std::string> &m,
    Cache<std::string, std::string> &cache)
{
    if (reply->type != REDIS_REPLY_ARRAY || (reply->elements & 1)) {
        LOG_WARN("reply is not array");
        return -1;
    }

    for (size_t i = 0; i < reply->elements / 2; i++) {
        int type = reply->element[i * 2 + 1]->type;
        LOG_DEBUG("element[%d] type = %d", i, type);

        if (type == REDIS_REPLY_STRING) {
            m[reply->element[i * 2]->str] = reply->element[i * 2 + 1]->str;
            cache.Set(reply->element[i * 2]->str,
                reply->element[i * 2 + 1]->str);
        } else {
            m[reply->element[i * 2]->str] = "";
            cache.Set(m[reply->element[i * 2]->str], "");
        }
    }
    return 0;
}

int FBServer::ReqAdvertiseCreative(uint64_t creative_id, std::string &cmd,
                                   SessData *sess)
{
    std::string creative_str = Func::to_str(creative_id);
    std::string *creative = creative_cache_.Get(creative_str);
    if (creative) {
        LOG_DEBUG("get creative in cache");
        sess->creativeMap[creative_str] = *creative;
        return 0;
    }

    LOG_DEBUG("get creative from redis");

    auto &creativeVt = sess->creativeVt;
    auto it = std::find(creativeVt.begin(), creativeVt.end(), creative_str);

    if (it == sess->creativeVt.end()) {
        cmd.append(" ");
        cmd.append(creative_str);
        creativeVt.push_back(creative_str);
    }
    return 0;
}

int FBServer::ReqAdvertiseGroup(uint32_t adgroup_id, std::string &cmd,
                                SessData *sess)
{
    std::string adgroup_str = Func::to_str(adgroup_id);

    /* if group id in cache, add the result to the groupMap but not Vector */
    std::string *group = group_cache_.Get(adgroup_str);
    if (group) {
        LOG_DEBUG("get group in cache");
        sess->adGroupMap[adgroup_str] = *group;
        return 0;
    }
    LOG_DEBUG("get group in cache");

    auto &adGroupVt = sess->adGroupVt;
    auto it = std::find(adGroupVt.begin(), adGroupVt.end(), adgroup_str);

    /* HMGET GAMES_AD_COST_AD:${today} ${adgroup1} ${adgroup2} ...,
     * e.g. "HMGET" "GAMES_AD_COST_CAMP:20161126" "101" "102" */
    if (it == adGroupVt.end()) {
        cmd.append(" ");
        cmd.append(adgroup_str);
        adGroupVt.push_back(adgroup_str);
    }
    return 0;
}

int FBServer::ReqAdvertiseCampaign(uint32_t campaign_id, std::string &cmd,
                                   SessData *sess)
{
    std::string campaign_str = Func::to_str(campaign_id);

    std::string *campaign = campaign_cache_.Get(campaign_str);
    if (campaign) {
        LOG_DEBUG("get campaign in cache");
        sess->campaignMap[campaign_str] = *campaign;
        return 0;
    }

    LOG_DEBUG("get campaign from redis");

    auto &campaignVt = sess->campaignVt;
    auto it = std::find(campaignVt.begin(), campaignVt.end(), campaign_str);

    /* HMGET GAMES_AD_COST_CAMP:${today} ${campaign1} ${campaign2} ... */
    if (it == campaignVt.end()) {
        cmd.append(" ");
        cmd.append(campaign_str);
        campaignVt.push_back(campaign_str);
    }
    return 0;
}

int FBServer::ReqAdvertiser(uint32_t advertiser_id, const std::string &lastday,
    std::string &cmd, std::string &yesCmd, SessData *sess)
{
    std::string advertiser_str = Func::to_str(advertiser_id);
    std::string *advertiser = advertiser_cache_.Get(advertiser_str);
    if (advertiser) {
        LOG_DEBUG("get advertiser in cache");
        sess->advertiserMap[advertiser_str] = *advertiser;
    } else {
        LOG_DEBUG("get advertiser from redis");

        auto &advertiserVt = sess->advertiserVt;
        auto it = std::find(
            advertiserVt.begin(), advertiserVt.end(), advertiser_str);

        /* HMGET GAMES_AD_COST_ADVER:${today} $adv1 $adv2 ... */
        if (it == advertiserVt.end()) {
            cmd.append(" ");
            cmd.append(advertiser_str);
            advertiserVt.push_back(advertiser_str);
        }
    }

    /* TODO: time this */
    if (lastdayAdvertiserMap_.find(lastday) == lastdayAdvertiserMap_.end()) {
        LOG_INFO("lastday %s not found, clean map", lastday.c_str());
        lastdayAdvertiserMap_.clear();
    }

    auto last_it = lastdayAdvertiserMap_[lastday].find(advertiser_str);
    if (last_it == lastdayAdvertiserMap_[lastday].end()) {

        auto &lastVt = sess->lastdayAdvertiserVt;

        /* HMGET GAMES_AD_COST_ADVER:${yesterday} $adv1 $adv2 ... */
        auto yes_it = std::find(lastVt.begin(), lastVt.end(),
                            advertiser_str);

        if (yes_it == lastVt.end()) {
            yesCmd.append(" ");
            yesCmd.append(advertiser_str);
            lastVt.push_back(advertiser_str);
        }
    }
    return 0;
}

int FBServer::ReqPdbDeal(const std::string &deal_id, int campaign_id,
                         std::string &dealCmd, std::string &campaignCmd,
                         SessData *sess)
{
    if (deal_id.size()) {
        common::FeedbackInfo_PdbFeedback *deal = deal_cache_.Get(deal_id);
        if (deal) {
            sess->dealMap[deal_id] = *deal;
        } else {
            auto &dealVt = sess->dealVt;
            auto it = std::find(dealVt.begin(), dealVt.end(), deal_id);

            if (it == dealVt.end()) {
                /* HMGET DEAL_ID:${today} ${deal_id1}, ${deal_id2}, ... */
                dealCmd.append(" ");
                dealCmd.append(deal_id);
                dealVt.push_back(deal_id);
            }
        }

        //交易推广计划
        std::string deal_campain_str;
        deal_campain_str.append(deal_id);
        deal_campain_str.append(MEM_STATISTICS_COM_DEF);
        deal_campain_str.append(Func::to_str(campaign_id));

        LOG_DEBUG("deal_campain_str[%s]", deal_campain_str.c_str());

        common::FeedbackInfo_PdbFeedback *deal_campaign =
            deal_campaign_cache_.Get(deal_campain_str);

        if (deal_campaign) {
            sess->dealCampaignMap[deal_campain_str] = *deal_campaign;
        } else {
            auto &dealCampaignVt = sess->dealCampaignVt;
            auto it = std::find(dealCampaignVt.begin(), dealCampaignVt.end(),
                           deal_campain_str);

            /* HMGET DEAL_CAMPAIGN_ID:${today} ${deal_id}:${campaign_id} */
            if (it == dealCampaignVt.end()) {
                campaignCmd.append(" ");
                campaignCmd.append(deal_campain_str);
                dealCampaignVt.push_back(deal_campain_str);
            }
        }
    } // end if deal_str
    return 0;
}

int FBServer::OnAdvertiseNum(redisReply *reply, SessData *sess)
{
    LOG_DEBUG("OnAdvNum");

    if (0 != CheckReply(reply))
        return -1;

    /* HGETALL */
    if (0 != GetMap(reply, sess->countMap, count_cache_))
        return -1;

    return 0;
}

int FBServer::OnAdvertiseCreative(redisReply *reply, SessData *sess)
{
    LOG_DEBUG("OnAdvCreative");

    if (0 != CheckReply(reply))
        return -1;

    if (0 != GetMap(reply, sess->creativeVt, sess->creativeMap,
        creative_cache_))
        return -1;

    return 0;
}

int FBServer::OnAdvertiseGroup(redisReply *reply, SessData *sess)
{
    LOG_DEBUG("OnAdvGroup");

    if (0 != CheckReply(reply))
        return -1;

    if (0 != GetMap(reply, sess->adGroupVt, sess->adGroupMap, group_cache_))
        return -1;

    return 0;
}

int FBServer::OnAdvertiseCampaign(redisReply *reply, SessData *sess)
{
    LOG_DEBUG("OnAdvCampaign");

    if (0 != CheckReply(reply))
        return -1;

    if (0 != GetMap(reply, sess->campaignVt, sess->campaignMap,
        campaign_cache_))
        return -1;

    return 0;
}

int FBServer::OnAdvertiser(redisReply *reply, SessData *sess)
{
    LOG_DEBUG("OnAdvertiser");

    if (0 != CheckReply(reply))
        return -1;

    if (0 != GetMap(reply, sess->advertiserVt, sess->advertiserMap,
        advertiser_cache_))
        return -1;

    return 0;
}

int FBServer::OnLastDayAdvertiser(const std::string &dt, redisReply *reply,
    SessData *sess)
{
    LOG_DEBUG("OnLastDayAdv lastday = %s", dt.c_str());

    if (0 != CheckReply(reply))
        return -1;

    if (reply->type != REDIS_REPLY_ARRAY) {
        LOG_DEBUG("OnLastDayAdv type error: %d", reply->type);
        return -1;
    }

    for (size_t i = 0; i < sess->lastdayAdvertiserVt.size(); i++) {
        int type = reply->element[i]->type;
        LOG_DEBUG("element[%d] type = %d", i, type);

        if (type == REDIS_REPLY_STRING) {
            lastdayAdvertiserMap_[dt][sess->lastdayAdvertiserVt[i]] =
                reply->element[i]->str;

            LOG_DEBUG("Lastday %s advertiser = %s, cost = %s", dt.c_str(),
                sess->lastdayAdvertiserVt[i].c_str(), reply->element[i]->str);
        } else {
            lastdayAdvertiserMap_[dt][sess->lastdayAdvertiserVt[i]] = "";

            LOG_DEBUG("Lastday %s advertiser = %s, cost = ?",
                sess->lastdayAdvertiserVt[i].c_str());
        }

    }
    return 0;
}

int FBServer::OnPdbDeal(redisReply *reply, SessData *sess)
{
    if (0 != CheckReply(reply))
        return -1;

    if (reply->type != REDIS_REPLY_ARRAY) {
        LOG_WARN("reply is not array");
        return -1;
    }

    for (size_t i = 0; i < reply->elements; i++) {
        int type = reply->element[i]->type;
        LOG_DEBUG("element[%d] type = %d", i, type);

        common::FeedbackInfo_PdbFeedback pdb;
        pdb.set_deal_id(sess->dealVt[i]);
        if (type == REDIS_REPLY_STRING)
            pdb.set_deal_day_exp(strtoll(reply->element[i]->str, NULL, 10));
        else
            pdb.set_deal_day_exp(0);
        sess->dealMap[sess->dealVt[i]] = pdb;
        deal_cache_.Set(sess->dealVt[i], pdb);
    }
    return 0;
}

int FBServer::OnPdbDealCampaign(redisReply *reply, SessData *sess)
{
    if (0 != CheckReply(reply))
        return -1;

    if (reply->type != REDIS_REPLY_ARRAY) {
        LOG_WARN("reply is not array");
        return -1;
    }

    for (size_t i = 0; i < reply->elements; i++) {
        int type = reply->element[i]->type;
        LOG_DEBUG("element[%d] type = %d", i, type);

        common::FeedbackInfo_PdbFeedback pdb;
        if (type == REDIS_REPLY_STRING) {
            pdb.set_deal_campaign_day_exp(
                strtoll(reply->element[i]->str, NULL, 10));
        } else {
            pdb.set_deal_campaign_day_exp(0);
        }
        sess->dealCampaignMap[sess->dealCampaignVt[i]] = pdb;
        deal_campaign_cache_.Set(sess->dealCampaignVt[i], pdb);
    }
    return 0;
}

int FBServer::Response(SessData *sess)
{
    int rt = 0;
    FeedbackRequest &fbreq = sess->fbreq;
    FeedbackResponse &fbrsp = sess->fbrsp;

    do {
        /* set session id and set trace id. */
        if (fbreq.has_session_id()) {
            fbrsp.set_session_id(fbreq.session_id());
        }

        if (fbreq.has_trace_id()) {
            fbrsp.set_trace_id(fbreq.trace_id());
        }

        /* message FeedbackResponse {
               optional bytes session_id = 1;
               required poseidon.common.ErrorCode error_code = 2;
               repeated poseidon.common.FeedbackInfo feedbackinfo = 3;
               optional string trace_id = 4;
           }

           message FeedbackInfo {
               optional uint32 adgroup_id = 1;               // 广告id
               optional uint32 campaign_id = 2;              // 推广计划id
               optional uint32 advertiser_id = 3;            // 广告主id
               optional string aid = 4;                      // ali id
               optional string acookie = 5;                  // ali cookie
               optional string dev_id = 6;                   // 设备ID
               optional int64 advertiser_day_cost = 7;       // 广告主当日花费
               optional int64 campaign_day_cost = 8;         // 推广计划当日话费
               optional int64 adgroup_day_cost = 9;          // 广告当日扣费
               optional int32 advertiser_user_day_freq = 10; // 广告主-用户
           当天频次次数
               optional int32 campaign_user_day_freq = 11;   // 广告计划-用户
           当天频次次数
               optional int32 adgroup_user_day_freq = 12;    // 广告-用户
           当天频次次数

               message CreativeCost {
                   optional int32 cid = 1;                   // 创意ID
                   optional int64 creative_day_cost = 2;     // 创意当日话费
                   optional int32 creative_day_freq = 3;     // 创意当天频次次数
               };
               repeated CreativeCost creative_cost = 13;     // 创意级别扣费情况

               //PDB 的反馈信息
               message PdbFeedback {
                   optional bytes deal_id = 1;               // 订单ID
                   optional int64 deal_day_exp = 2;          // 订单单天曝光量
                   optional int64 deal_campaign_day_exp = 3; //
           该订单的该推广计划曝光情况
               };
               optional PdbFeedback pdb_feedback = 14;
               optional int64 advertiser_last_day_cost = 15; //
           广告主昨天消费金额
           } */

        for (int i = 0; i < sess->fbreq.ad_size(); i++) {

            common::FeedbackInfo *fb;
            fb = sess->fbrsp.add_feedbackinfo();

            if (sess->fbreq.ad(i).has_pdb_data()) {
                std::string dealId = sess->fbreq.ad(i).pdb_data().deal_id();

                if (dealId.size()) {
                    /* dealMap[dealId] => FeedbackInfo_PdbFeedback
                     * see OnPdbDeal for detail. */
                    common::FeedbackInfo_PdbFeedback pdb =
                        sess->dealMap[dealId];
                    dealId.append(":");
                    dealId.append(
                        Func::to_str(fbreq.ad(i).campaign_id()));

                    LOG_INFO("deal:campId[%s]\n", dealId.c_str());
                    common::FeedbackInfo_PdbFeedback camp_pdb =
                        sess->dealCampaignMap[dealId.c_str()];

                    LOG_INFO("camp_pdb.deal_campaign_day_exp()[%d]\n",
                             camp_pdb.deal_campaign_day_exp());

                    pdb.set_deal_campaign_day_exp(
                        camp_pdb.deal_campaign_day_exp());
                    fb->mutable_pdb_feedback()->CopyFrom(pdb);
                }
            }

            uint32_t adgroup_id = sess->fbreq.ad(i).adgroup_id();
            uint32_t campaign_id = sess->fbreq.ad(i).campaign_id();
            uint32_t advertiser_id = sess->fbreq.ad(i).inner_advertiser_id();
            uint64_t creative_id = sess->fbreq.ad(i).creative_id();
            const std::string &aid = sess->fbreq.aid();
            const std::string &acookie = sess->fbreq.acookie();
            const std::string &dev_id = sess->fbreq.dev_id();

            fb->set_adgroup_id(adgroup_id);
            fb->set_campaign_id(campaign_id);
            fb->set_advertiser_id(advertiser_id);

            fb->set_aid(aid);
            fb->set_acookie(acookie);
            fb->set_dev_id(dev_id);

            std::string memAdvertister =
                sess->advertiserMap[Func::to_str(advertiser_id)];

            if (memAdvertister.size()) {
                fb->set_advertiser_day_cost(
                    strtoull(memAdvertister.c_str(), NULL, 0) / 1000L);
            } else {
                fb->set_advertiser_day_cost(0);
            }

            std::string memCampaign =
                sess->campaignMap[Func::to_str(campaign_id)];

            LOG_DEBUG("memCampaign[%s]=", memCampaign.c_str());
            if (memCampaign.size()) {
                LOG_DEBUG("memCampaign[%s]=", memCampaign.c_str());
                fb->set_campaign_day_cost(
                    strtoll(memCampaign.c_str(), NULL, 10) / 1000L);
            } else {
                fb->set_campaign_day_cost(0);
            }

            std::string memAd =
                sess->adGroupMap[Func::to_str(adgroup_id)];
            if (memAd.length()) {
                fb->set_adgroup_day_cost(strtoll(memAd.c_str(), NULL, 10) /
                                         1000L);
            } else {
                fb->set_adgroup_day_cost(0);
            }

            std::string advertiserCount =
                MEM_KEY_ADVER + Func::to_str(advertiser_id);

            std::string mem_adver_count = sess->countMap[advertiserCount];
            if (mem_adver_count.size()) {
                fb->set_advertiser_user_day_freq(atol(mem_adver_count.c_str()));
            } else {
                fb->set_advertiser_user_day_freq(0);
            }

            std::string campaignCount =
                MEM_KEY_CAMP + Func::to_str(campaign_id);

            std::string mem_camp_count = sess->countMap[campaignCount];

            if (mem_camp_count.size()) {
                fb->set_campaign_user_day_freq(atol(mem_camp_count.c_str()));
            } else {
                fb->set_campaign_user_day_freq(0);
            }

            if (!sess->fbreq.ad(i).advertiser_balance_day().empty()) {
                fb->set_advertiser_last_day_cost(
                    strtoll(lastdayAdvertiserMap_
                                [sess->last_day]
                                [Func::to_str(advertiser_id)].c_str(),
                            NULL, 10) /
                    1000L);
            }

            std::string adgroupCount =
                MEM_KEY_AD + Func::to_str((uint64_t)adgroup_id);

            std::string mem_ad_count = sess->countMap[adgroupCount];
            if (mem_ad_count.size()) {
                fb->set_adgroup_user_day_freq(
                    Func::to_int(mem_ad_count.c_str()));
            } else {
                fb->set_adgroup_user_day_freq(0);
            }

            // 创意...
            if (fbreq.ad(i).has_creative_id()) {
                common::FeedbackInfo::CreativeCost *creativeCost =
                    fb->add_creative_cost();

                //创意次数
                std::string mem_creative_count =
                    MEM_KEY_CREATIVE + Func::to_str(creative_id);

                std::string creative_cost =
                    sess->creativeMap[Func::to_str(creative_id)];
                std::string creative_ad_count =
                    sess->countMap[mem_creative_count];

                /* temporarily don't need these data in FBResponse,
                 * so make them all 0. */

                creativeCost->set_cid(creative_id);
                creativeCost->set_creative_day_cost(0);
                creativeCost->set_creative_day_freq(0);

//                if (creative_cost.size()) {
//                    creativeCost->set_creative_day_cost(
//                        strtoll(creative_cost.c_str(), NULL, 10) / 1000L);
//                } else {
//                    creativeCost->set_creative_day_cost(0);
//                }

//                if (creative_ad_count.size()) {
//                    creativeCost->set_creative_day_freq(
//                        Func::to_int(creative_ad_count.c_str()));
//                } else {
//                    creativeCost->set_creative_day_freq(0);
//                }
//                creativeCost->set_cid(creative_id);
            }
        }

        sess->fbrsp.set_error_code(common::ERROR_NONE);
        std::string sendbuf;
        if (!fbrsp.SerializeToString(&sendbuf)) {
            MON_ADD(ATTR_PACK_ERROR, 1);
            LOG_ERROR("SerializeToString error trance_id[%s]",
                      fbreq.trace_id().c_str());
            rt = -1;
            break;
        }
        LOG_DEBUG("dnrsp[%s]\n", fbrsp.DebugString().c_str());

        if (!Config::get_mutable_instance().IsDumb()) {
            WriteBytes(sess->host, sess->port, sendbuf.c_str(),
                       sendbuf.length());
            MON_ADD(ATTR_FB_RSP, 1);
        }

    } while (0);

    return rt;
}

} // namespace poseidon
} // namespace feedback
