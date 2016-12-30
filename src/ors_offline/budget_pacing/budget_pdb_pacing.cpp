/**
**/


#include <fstream>
#include <cstdlib>
#include <string>
#include <math.h>

#include "json/json.h"
#include "../util/log.h"
#include "../third_party/hiredis/include/hiredis.h"
#include "boost/algorithm/string.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/format.hpp"
#include "types.h"

#include "../common/utility.h"
#include "campaign_budget.h"
#include "budget_pdb_pacing.h"
#include "poseidon_ors_offline.pb.h"


int getPushRatio(const poseidon::ors_offline::BudgetPacingConfig &config, const time_t &t, const poseidon::ors_offline::pdbCampaignBudget &pdbCampaignBudget, map<string, set<UInt32> > &mp_deal2campaign, float &pushratio);

float getDealExp(const poseidon::ors_offline::BudgetPacingConfig &config, const time_t &t, const poseidon::ors_offline::pdbCampaignBudget &pdbCampaignBudget, UInt32 &past_budget);

int getPDBDealBudgets(const string &file_path, vector<poseidon::ors_offline::pdbCampaignBudget> &campaign_budgets, const time_t &t, int compaign_type, map<string, set<UInt32> > &mp_deal2campaign) {

    LOG_INFO("get PDB CampaignBudgets beginning.");

    ifstream in(file_path.c_str(), ios::in | ios::binary);
    if (!in) {
        LOG_ERROR("Failed to open file %s", file_path.c_str());
        return -1;
    }

    string line;

    Json::Reader reader;

    while (getline(in, line)) {

        Json::Value value;

        if (reader.parse(line, value)) {

            // start pdb data
            string pdb_data("pdb_data");

            if (!value.isMember(pdb_data.c_str())) {
                LOG_ERROR("Ads info do not have pdb_data. %s", line.c_str());
                continue;
            }

            string type("compaign_type");
            if (value.isMember(type.c_str()) && value[type.c_str()].asInt() != compaign_type) {

                LOG_INFO("Ad info compaign type is PDB, compaign_type is %d", value[type.c_str()].asInt());

                continue;
            }

            poseidon::ors_offline::pdbCampaignBudget campaign_budget;

            string campaign_id_key("campaign_id");

            if (!value.isMember(campaign_id_key.c_str())) {
                LOG_ERROR("Ad info do not have campaign_id. %s", line.c_str());
                continue;
            }

            /**
             * UInt32 compaign_id;
             * UInt32 target;
             * UInt32 m_ad_time_type;
             * std::string m_post_hours;
             */

            UInt32 campaign_id = value["campaign_id"].asUInt();

            campaign_budget.setCompaign_id(campaign_id);

            string ad_time_type_key("ad_time_type");
            if (!value.isMember(ad_time_type_key.c_str())) {
                LOG_ERROR("Ads info do not have ad_time_type. %s", line.c_str());
                continue;
            }
            campaign_budget.setM_ad_time_type(value[ad_time_type_key.c_str()].asUInt());

            if (campaign_budget.getM_ad_time_type() == ::poseidon::ors_offline::SELECT_HALF_HOURS)//2)
            {
                string post_hours_key("post_hours");
                if (!value.isMember(post_hours_key.c_str())) {
                    LOG_ERROR("Ads info do not have post_hours. %s", line.c_str());
                    continue;
                }
                campaign_budget.setM_post_hours(value[post_hours_key.c_str()].asString(), get_week_day(t));
            }

            string target_adx_key("source");

            if (!value.isMember(target_adx_key.c_str())) {
                LOG_ERROR("Ads info do not have target_adx(source). %s", line.c_str());
                continue;
            }
            campaign_budget.setTarget(value[target_adx_key.c_str()].asUInt());


            /**
            std::string deal_id;
            UInt64 exp;
            UInt32 quota;
            UInt32 fillrate;
            */
            string deal_id("deal_id");
            // change from campaign day exp to deal total exp
            // TODO: need to confirm
            string day_exp("day_exp");
            string campaign_quota("campaign_quota");
            string fill_rate("fill_rate");

            if (
                    !value[pdb_data.c_str()].isMember(deal_id.c_str()) ||
                    !value[pdb_data.c_str()].isMember(day_exp.c_str()) ||
                    !value[pdb_data.c_str()].isMember(campaign_quota.c_str()) ||
                    !value[pdb_data.c_str()].isMember(fill_rate.c_str())) {
                LOG_ERROR( "ad does not have required info. %s", line.c_str());
                continue;
            }

            deal_id = value[pdb_data.c_str()][deal_id.c_str()].asString();

            UInt32 exp = value[pdb_data.c_str()][day_exp.c_str()].asUInt();

            UInt32 quota = value[pdb_data.c_str()][campaign_quota.c_str()].asUInt();

            UInt32 fillrate = value[pdb_data.c_str()][fill_rate.c_str()].asUInt();

            campaign_budget.setDeal_id(deal_id);

            campaign_budget.setExp(exp);

            campaign_budget.setQuota(quota);

            campaign_budget.setFillrate(fillrate);


            // update operation
            map<string, set<UInt32> >::iterator it_dealcampaign = mp_deal2campaign.find(campaign_budget.getDeal_id());
            if (mp_deal2campaign.end() == it_dealcampaign){
                    set<UInt32> set_campaign;
                    set_campaign.insert(campaign_budget.getCompaign_id());
                    mp_deal2campaign.insert(pair<string, set<UInt32> > (campaign_budget.getDeal_id(), set_campaign));
                    LOG_INFO("pdb one campaign was inserted into mp_deal2campaign");
            }else{
                   it_dealcampaign->second.insert(campaign_budget.getCompaign_id());
                    LOG_INFO("pdb one campaign %d was inserted into deal %s mp_deal2campaign", campaign_budget.getCompaign_id(), campaign_budget.getDeal_id().c_str());
            }


            campaign_budgets.push_back(campaign_budget);


        }
        in.close();

        LOG_INFO("get PDB CampaignBudgets end successfully.");
    }
        return 0;
}

float getForecastPacingChangeRate(const poseidon::ors_offline::BudgetPacingConfig &config,
                                  map<UInt32, int> &campaign_posted_budget,
                                  map<UInt32, float> &history_posted_budget_smooth,
                                  poseidon::ors_offline::pdbCampaignBudget &campaign_budget,
                                  time_t t,
                                  int posted_budget) {
        LOG_INFO("getForecastPacingChangeRate beginning.");

        float pacing_change_rate = 0.0f;

        map<UInt32, int>::iterator it = campaign_posted_budget.find(campaign_budget.getCompaign_id());

        if (it == campaign_posted_budget.end()) {
            pacing_change_rate = 0.0f;
            campaign_posted_budget[campaign_budget.getCompaign_id()] = posted_budget;
            LOG_DEBUG("There is no accumulate posted budget for last minute. ==> pacing_change_rate=%f", pacing_change_rate);
            return pacing_change_rate;
        }

        int last_acc_posted_budget = it->second;
        float last_mins_posted_budget = (float) (posted_budget - last_acc_posted_budget);
        map<UInt32, float>::iterator his_it = history_posted_budget_smooth.find(campaign_budget.getCompaign_id());
        if (his_it != history_posted_budget_smooth.end()) {
            last_mins_posted_budget = 0.5 * last_mins_posted_budget + 0.5 * his_it->second;
            LOG_DEBUG("Smoothing last minute posted budget:last_mins_posted_budget_smoothed=%f, last_mins_posted_budget=%f, history_posted_budget=%f.",last_mins_posted_budget, (float) (posted_budget - last_acc_posted_budget), his_it->second);
        }else{
            LOG_DEBUG("Has no smoothed history posted budget data, last_mins_posted_budget=%f", last_mins_posted_budget);
        }

        campaign_posted_budget[campaign_budget.getCompaign_id()] = posted_budget;
        history_posted_budget_smooth[campaign_budget.getCompaign_id()] = last_mins_posted_budget;
        if (last_mins_posted_budget < 0) {
            pacing_change_rate = 0.0f;
            LOG_ERROR("last_mins_posted_budget < 0: last_mins_posted_budget=%f, last_acc_posted_budget=%d, posted_budget=%d ==>pacing_change_rate=%f.", last_mins_posted_budget, last_acc_posted_budget, posted_budget, pacing_change_rate);
            campaign_posted_budget.erase(campaign_budget.getCompaign_id());
            history_posted_budget_smooth.erase(campaign_budget.getCompaign_id());
            return pacing_change_rate;
        } else if ( last_mins_posted_budget == 0 ) {
            pacing_change_rate = 1.0f;
            LOG_DEBUG("last_mins_posted_budget == 0: last_mins_posted_budget=%f, last_acc_posted_budget=%d, posted_budget=%d ==> pacing_change_rate=%f.",last_mins_posted_budget, last_acc_posted_budget, posted_budget, pacing_change_rate);
            return pacing_change_rate;
        }

        int total_posted_time = campaign_budget.get_total_posted_time(get_mins_of_day(t));
        int total_schedule_time = campaign_budget.get_total_schedule_time();
        int remaining_time = total_schedule_time - total_posted_time;
        if (remaining_time <= 0) {
            pacing_change_rate = 0.0f;
            LOG_DEBUG("remaining_time <= 0: remaining_time=%d, total_posted_time=%d, total_schedule_time=%d ==> pacing_change_rate=%f",remaining_time, total_posted_time, total_schedule_time, pacing_change_rate);
            return pacing_change_rate;
        }

        int total_budget = campaign_budget.getExp();
        int remaining_budget = total_budget - posted_budget;
        if (remaining_budget <= 0) {
            pacing_change_rate = 0.0f;
            LOG_DEBUG("remaining_budget <= 0: remaining_budget=%d, total_budget=%d, posted_budget=%d ==> pacing_change_rate=%f", remaining_budget, total_budget, posted_budget, pacing_change_rate);
            return pacing_change_rate;
        }

        float mins_schedule_budget = (float) (remaining_budget) / (float) remaining_time;
        LOG_DEBUG("Calculate mins_schedule_budget based on remaining budget and time:mins_schedule_budget=%f, remaining_budget=%d, remaining_time=%d.",mins_schedule_budget, remaining_budget, remaining_time);
        if (mins_schedule_budget > last_mins_posted_budget) {
            pacing_change_rate = (mins_schedule_budget - last_mins_posted_budget) / last_mins_posted_budget;
            LOG_DEBUG("pacing_change_rate=%f, mins_schedule_budget=%f, last_mins_posted_budget=%f", pacing_change_rate, mins_schedule_budget, last_mins_posted_budget);
            if (pacing_change_rate > 1.0f) {
                LOG_DEBUG("Adjust pacing_change_rate to 1.0f");
                pacing_change_rate = 1.0f;
            }
        } else if (mins_schedule_budget < last_mins_posted_budget) {
            pacing_change_rate = (mins_schedule_budget - last_mins_posted_budget) / mins_schedule_budget;
            LOG_DEBUG("pacing_change_rate=%f, mins_schedule_budget=%f, last_mins_posted_budget=%f", pacing_change_rate,mins_schedule_budget, last_mins_posted_budget);
            if (pacing_change_rate < -0.5f) {
                LOG_DEBUG("Adjust pacing_change_rate to -0.5f");
                pacing_change_rate = -0.5f;
            }
        }
        LOG_INFO("getForecastPacingChangeRate ended successfully(pacing_change_rate=%f).", pacing_change_rate);
        return pacing_change_rate;
}

/**
 *
 * @param item
 * @param deal_id
 * @param bidding_rate
 * @param budget_exceeding_ratio
 * @param target_adx
 * @param fixed_price
 */
void addBudgetPacingItem(poseidon::ors::BudgetPacingItem *item,
                         UInt32 campaign_id,
                         string deal_id,
                         float bidding_rate,
                         float budget_exceeding_ratio,
                         UInt32 target_adx) {
    item->set_campaign_id(campaign_id);
    item->set_deal_id(deal_id);
    item->set_budget_pacing_ratio(bidding_rate);
    item->set_budget_exceeding_ratio(budget_exceeding_ratio);
    item->set_adx_id(target_adx);

}


/**
 *  access Redis and get push ratio
 * @param config
 * @param t
 * @param campaign_id
 * @param deal_id
 * @param source
 * @param mp_deal2campaign
 * @param pushratio
 * @return
 */
int getPushRatio(const poseidon::ors_offline::BudgetPacingConfig &config,
                 const time_t &t,
                 poseidon::ors_offline::pdbCampaignBudget &pdbCampaignBudget, 
                 map<string, set<UInt32> > &mp_deal2campaign,
                 float &pushratio) {

    char day[9];
    strftime(day, 9, "%Y%m%d", localtime(&t));

    LOG_INFO("get push ratio beginning.");

    redisContext *c;

    struct timeval timeout = {1, 500000};

    string redis_ip = config.redis_server().ip();
    int redis_port = config.redis_server().port();

    c = redisConnectWithTimeout(redis_ip.c_str(), redis_port, timeout);
    if (c == NULL || c->err) {
        if (c) {
            LOG_ERROR("Connection error: %s. redis_server_ip=%s, redis_server_port=%d.", c->err, redis_ip.c_str(), redis_port);
        } else {
            LOG_ERROR("Connection error: can't allocate redis context. redis_server_ip=%s, redis_server_port=%d.", redis_ip.c_str(), redis_port);
        }
        return -1;
    }

    /**************start for bidding cnt********************/
    LOG_INFO("pdb start caculate the bidding cnt from redis");
    int bidcnt = 0;
    string ymd(day);
    string deal_id(pdbCampaignBudget.getDeal_id());
    string source(boost::lexical_cast<string>(pdbCampaignBudget.getTarget()));

    map<string, set<UInt32> >::iterator it = mp_deal2campaign.find(pdbCampaignBudget.getDeal_id());
    if ( it == mp_deal2campaign.end() ){
        LOG_ERROR("such campaign %d isnot in any deal, maybe error occurred in json parse", pdbCampaignBudget.getCompaign_id());
        return -1;
    }

    for( set<UInt32>::iterator it_campaignset = it->second.begin(); it_campaignset != it->second.end(); ++it_campaignset){

        string campaign_id(boost::lexical_cast<string>(*it_campaignset));

        LOG_INIT("pdb begin iterator the campaignset, campaign_id is %s", campaign_id.c_str());

        /*
        * 推广计划-订单-出价PV
        * 主KEY：rts_ad_deal_bid_pv:{YYYYMMDD}0000:1-D:campaign_id`deal_id`source
        * 子KEY：deal_bid_pv:{campaign_id}`{deal_id}`{source}
        *
        * 订单-请求量PV
        * 主KEY:rts_ad_deal_req_pv:{YYYYMMDD}0000:1-D:deal_id`source
        * 子KEY:deal_req_pv:{deal_id}`{source}
        */

        string masterkey("rts_ad_deal_bid_pv:" + ymd + "0000:1-D:campaign_id`deal_id`source");
        string subkey("deal_bid_pv:" + campaign_id + "`" + deal_id + "`" + source);

        string redis_cmd("hget " + masterkey + " " + subkey);

        LOG_INFO("pdb bid redis_cmd is %s", redis_cmd.c_str());

        redisReply *reply = (redisReply *) redisCommand(c, redis_cmd.c_str());

        LOG_DEBUG("Execute redis command:%s.", redis_cmd.c_str());
        if (reply == NULL) {
            LOG_ERROR("Cannot get realtime pdb stat from redis.");
            redisFree(c);
            return -1;
        }

        if (reply->str == NULL) {
            LOG_ERROR("Redis reply string is NULL.");
            freeReplyObject(reply);
            redisFree(c);
            return -1;
        }

        bidcnt += atoi(reply->str);
        freeReplyObject(reply);
    }

    LOG_INFO("pdb bidcnt number is %d", bidcnt);
    /**************end for bidding cnt********************/

    string reqmasterkey("rts_ad_deal_req_pv:" + ymd + "0000:1-D:deal_id`source");
    string reqsubkey("deal_req_pv:" + deal_id + "`" + source);
    string redis_req_cmd("hget " + reqmasterkey + " " + reqsubkey);


    LOG_INFO("pdb req redis_cmd is %s", redis_req_cmd.c_str());

    redisReply *reqreply = (redisReply *) redisCommand(c, redis_req_cmd.c_str());

    LOG_DEBUG("Execute redis command:%s.", redis_req_cmd.c_str());

    if (reqreply == NULL) {
        LOG_ERROR("Cannot get realtime pdb stat from redis.");
        redisFree(c);
        return -1;
    }

    if (reqreply->str == NULL) {
        LOG_ERROR("Redis reply string is NULL.");
        freeReplyObject(reqreply);
        redisFree(c);
        return -1;
    }

    int reqcnt = atoi(reqreply->str);
    LOG_INFO("pdb reqcnt number is %d", reqcnt);

    pushratio = (float) bidcnt / reqcnt;

    LOG_INFO("pdb push ratio is %f", pushratio);

    freeReplyObject(reqreply);

    redisFree(c);
    LOG_INFO("getUIntFromRedis ended successfully.");

    return 0;
}

/**
 *
 * @param config
 * @param t
 * @param campaignBudget
 * @param quotaratio
 * @param past_budget
 * @return
 */
float getDealExp(  const poseidon::ors_offline::BudgetPacingConfig &config,
                   const time_t &t,
                   const poseidon::ors_offline::pdbCampaignBudget &campaignBudget,
                   UInt32 &past_budget) {

    char day[9];
    strftime(day, 9, "%Y%m%d", localtime(&t));

    LOG_INFO("get quota ratio beginning.");

    redisContext *c;

    struct timeval timeout = {1, 500000};

    string redis_ip = config.redis_server().ip();
    int redis_port = config.redis_server().port();

    c = redisConnectWithTimeout(redis_ip.c_str(), redis_port, timeout);

    if (c == NULL || c->err) {
        if (c) {
            LOG_ERROR("Connection error: %s. redis_server_ip=%s, redis_server_port=%d.", c->err, redis_ip.c_str(), redis_port);
        } else {
            LOG_ERROR("Connection error: can't allocate redis context. redis_server_ip=%s, redis_server_port=%d.", redis_ip.c_str(), redis_port);
        }
        return -1;
    }

    // corresponding to a deal
    // DEAL_ID:20160909  100  1000

    string deal(campaignBudget.getDeal_id());
    string campaign(boost::lexical_cast<string>(campaignBudget.getCompaign_id()));
    string ymd(day);

    string dealkey("DEAL_ID:" + ymd);
    string dealsubkey(deal);

    string redis_deal_cmd("hget " + dealkey + " " + dealsubkey);

    LOG_INFO("pdb %s redis_cmd is %s", dealkey.c_str(), redis_deal_cmd.c_str());

    redisReply *dealreply = (redisReply *) redisCommand(c, redis_deal_cmd.c_str());

    LOG_DEBUG("Execute redis command:%s.", redis_deal_cmd.c_str());

    if (dealreply == NULL) {
        LOG_ERROR("Cannot get realtime pdb stat from redis.");
        redisFree(c);
        return -1;
    }

    if (dealreply->str == NULL) {
        LOG_ERROR("Redis reply string is NULL.");
        freeReplyObject(dealreply);
        redisFree(c);
        return -1;
    }

    int exp_deal_cnt = atoi(dealreply->str);

    LOG_INFO("pdb %s number is %d", dealkey.c_str(), exp_deal_cnt);
    freeReplyObject(dealreply);
    redisFree(c);

    return 0;
}


/**
 *
 * @param config
 * @param t
 * @param campaignBudget
 * @param quotaratio
 * @param past_budget
 * @return
 */
float getCampaignExp(  const poseidon::ors_offline::BudgetPacingConfig &config,
                   const time_t &t,
                   const poseidon::ors_offline::pdbCampaignBudget &campaignBudget,
                   UInt32 &past_budget) {

    char day[9];
    strftime(day, 9, "%Y%m%d", localtime(&t));

    LOG_INFO("get quota ratio beginning.");

    redisContext *c;

    struct timeval timeout = {1, 500000};

    string redis_ip = config.redis_server().ip();
    int redis_port = config.redis_server().port();

    c = redisConnectWithTimeout(redis_ip.c_str(), redis_port, timeout);

    if (c == NULL || c->err) {
        if (c) {
            LOG_ERROR("Connection error: %s. redis_server_ip=%s, redis_server_port=%d.", c->err, redis_ip.c_str(), redis_port);
        } else {
            LOG_ERROR("Connection error: can't allocate redis context. redis_server_ip=%s, redis_server_port=%d.", redis_ip.c_str(), redis_port);
        }
        return -1;
    }

    // corresponding to a deal
    // DEAL_ID:20160909  100  1000

    string deal(campaignBudget.getDeal_id());
    string campaign(boost::lexical_cast<string>(campaignBudget.getCompaign_id()));
    string ymd(day);

    string dealcampaignkey("DEAL_CAMPAIGN_ID:" + ymd);
    string dealcampaignsubkey(deal + ":" + campaign);

    string redis_deal_cmd("hget " + dealcampaignkey + " " + dealcampaignsubkey);

    LOG_INFO("pdb %s redis_cmd is %s", dealcampaignkey.c_str(), redis_deal_cmd.c_str());

    redisReply *dealreply = (redisReply *) redisCommand(c, redis_deal_cmd.c_str());

    LOG_DEBUG("Execute redis command:%s.", redis_deal_cmd.c_str());
    if (dealreply == NULL) {
        LOG_ERROR("Cannot get realtime pdb stat from redis.");
        redisFree(c);
        return -1;
    }

    if (dealreply->str == NULL) {
        LOG_ERROR("Redis reply string is NULL.");
        freeReplyObject(dealreply);
        redisFree(c);
        return -1;
    }

    past_budget = atoi(dealreply->str);

    LOG_INFO("pdb %s number is %d", dealcampaignkey.c_str(), past_budget);

    freeReplyObject(dealreply);
    redisFree(c);

    return 0;
}

/*
 * NOTE:
 * For compatibility type: map<UInt32, float> &campaign_bidding_rate
 * each bidding rate of campaign is the deal's rate.
 *
 */
int updatePDBBudgetPacingModel(const poseidon::ors_offline::BudgetPacingConfig &config,
                               map<UInt32, float> &campaign_bidding_rate,
                               map<UInt32, int> &campaign_posted_budget,
                               map<UInt32, float> &history_posted_budget_smooth,
                               vector<poseidon::ors_offline::pdbCampaignBudget> &campaign_budgets,
                               time_t t,
                               poseidon::ors::BudgetPacingModel &budget_pacing_model,
                               map<string, set<UInt32> > &mp_deal2campaign ) {

    LOG_INFO("update pdb BudgetPacingModel beginning.");

    for (vector<poseidon::ors_offline::pdbCampaignBudget>::iterator it = campaign_budgets.begin(); it != campaign_budgets.end(); ++it) {

        char day[9];

        strftime(day, 9, "%Y%m%d", localtime(&t));

        UInt32 target_adx = it->getTarget();

        float bidding_rate = 0.0f;

        float budget_exceeding_ratio = 0.0f;

        UInt32 compaign_id = it->getCompaign_id();

        string deal_id = it->getDeal_id();

        int deal_day_exp = it->getExp();
        UInt32 past_budget = 0;
        // plan to expend exp till now
        double schedule_budget = it->get_schedule_budget(get_mins_of_day(t));

        // pdb schedule time: campaign's is from deal's
        if ( it->is_during_schedule_time(get_mins_of_day(t)) ){

            //enter the update logic
            LOG_INFO("deal_id: %s, campaign_id: %d start update function.", deal_id.c_str(), compaign_id);

            map<UInt32, float>::iterator it_pastbidingrate = campaign_bidding_rate.find(it->getCompaign_id());

            if (it_pastbidingrate == campaign_bidding_rate.end()){

                LOG_INFO("past bidding rate doesn't exist, init now");

                bidding_rate = (float) it->getFillrate() * it->getQuota() / ( 100 * 1000 );

                campaign_bidding_rate.insert(pair<UInt32, float>(it->getCompaign_id(), bidding_rate));

                LOG_INFO("pdb init bidding rate is %f", bidding_rate);

            }else{

                bidding_rate = it_pastbidingrate->second;
                LOG_INFO("last 1 minute the bidding rate is %f", bidding_rate);

            }

            // Alpha
            // a deal is corresponding to only one fillrate
            // real time pushratio = (rts_ad_deal_bid_pv / rts_ad_deal_req_pv) > deal_id fillrate
            double alpha = 1.0;
            /**
             * Key Definition
             * 主KEY：rts_ad_deal_bid_pv:{YYYYMMDD}0000:1-D:campaign_id`deal_id`source
             * 子KEY：deal_bid_pv:{campaign_id}`{deal_id}`{source}
             * Formula:
             * 
             **/

            float pushratio = 0.0f;
            double alpha_coef = config.pdb_model_params().alpha_coefficient();
            double alpha_min  = config.pdb_model_params().alpha_min_value();
            double alpha_max  = config.pdb_model_params().alpha_max_value();

            if (-1 == getPushRatio(config, t, *it, mp_deal2campaign,  pushratio)) {
                LOG_ERROR("pdb get push ratio occur exception, maybe not start");
                bidding_rate = (float) it->getFillrate() * it->getQuota() / ( 100 * 1000) ;
                LOG_INFO("pdb alpha: when pushratio return unexpected: biddingrate set default %f", bidding_rate);
            }else{

                if (pushratio > (float) it->getFillrate() / 100.0) {

                    alpha = 1.0;

                    LOG_INFO("pdb push ratio is bigger than fillrate");
                } else {

                    double x = pushratio * 100 / it->getFillrate() * 100;

                    alpha = alpha_coef * log2(ceil(x) + 1);

                    LOG_INFO("pdb alpha is %f", alpha);

                    if (alpha < alpha_min ) {
                        alpha = alpha_min;
                        LOG_INFO("pdb alpha: %f is less than alpha min: %f", alpha, alpha_min);
                    }

                    if (alpha > alpha_max ) {
                        alpha = alpha_max;
                        LOG_INFO("pdb alpha: %f is bigger than alpha max: %f", alpha, alpha_max);
                    }
                    bidding_rate *= alpha;
                    LOG_INFO("pdb push ratio is less than fillrate. alpha is %f, bidding rate is %f", alpha, bidding_rate);

                }
            }

            it->setExp( it->getExp() * alpha );

            if ( 0 != getCampaignExp(config, t, *it, past_budget)){
                LOG_ERROR("pdb such campaign: %d occures exception when get exp", compaign_id);
            }else{
                LOG_INFO("pdb such campaign: %d has expended: %d", compaign_id, past_budget);
                float pacing_change_rate = config.pdb_model_params().pacing_change_rate();
                if ( config.pdb_model_params().dynamic_pacing_change_rate() == 1 ){
                    LOG_DEBUG("pdb Using dynamic_pacing_change_rate.");
                    pacing_change_rate =  getForecastPacingChangeRate(config, campaign_posted_budget, history_posted_budget_smooth, *it, t, past_budget);
                    LOG_DEBUG("Dynamic adjust bidding_rate with pacing_change_rate(=%f), bidding_rate=(before=%f, after=%f)",pacing_change_rate, bidding_rate, bidding_rate * (1 + pacing_change_rate));
                    bidding_rate *= (1 + pacing_change_rate);
                } else {
                    LOG_DEBUG("Using static pacing change rate.");
                    if ((double) past_budget < schedule_budget) {
                        bidding_rate *= (1 + pacing_change_rate);
                    } else if ((double) past_budget > schedule_budget){
                        bidding_rate *= (1 - pacing_change_rate);
                    }
                }
            }

            //TODO: qps need to confirm
            //
            if (bidding_rate >= 1) {
                LOG_DEBUG("Rescale bidding_rate to 1.0f");
                bidding_rate = 1;
            }


            // for: budget_exceeding_ratio
            if (schedule_budget > 0) {

                budget_exceeding_ratio = (float) past_budget / (float) deal_day_exp;

                if (budget_exceeding_ratio >= 1) {
                    bidding_rate = (double) it->getFillrate() * it->getQuota() / ( 100 * 1000 );
                }

            } else {
                budget_exceeding_ratio = 0;
                bidding_rate = (double) it->getFillrate() * it->getQuota() / ( 100 * 1000 );
            }

            campaign_bidding_rate[it->getCompaign_id()] = bidding_rate;

            if ( it->getExp() <= 0 || bidding_rate <= 0.0f ){
                map<UInt32, float>::iterator map_it = campaign_bidding_rate.find(it->getCompaign_id());
                if (map_it != campaign_bidding_rate.end()) {
                    campaign_bidding_rate.erase(map_it);
                }
            }

        }else{
             // NOT IN SCHEDULE TIME
            // ERASE FROM campaign_bidding_rate IF last 1 minute has exp.

            map<UInt32, float>::iterator map_it = campaign_bidding_rate.find(it->getCompaign_id());
            if (map_it != campaign_bidding_rate.end()) {
                campaign_bidding_rate.erase(map_it);
                LOG_INFO("pdb campaign_id %d not in schedule.", it->getCompaign_id());
            }

            //set both bidding_rate and budget_exceeding_ratio are 0
            bidding_rate = 0.0;
            budget_exceeding_ratio = 0.0;
        }

        addBudgetPacingItem(budget_pacing_model.add_items(), it->getCompaign_id(), it->getDeal_id(), bidding_rate, budget_exceeding_ratio, target_adx);

        LOG_DEBUG("deal_id=%s, campaign_id=%d; day_exp=%u; past_budget=%f; future_avg_budget=%d; bidding_rate=%f; budget_exceeding_ratio=%f, target_adx=%d.", deal_id.c_str(), it->getCompaign_id(), it->getExp(), past_budget, schedule_budget, bidding_rate, budget_exceeding_ratio, target_adx); 
    }

    return 0;

}
