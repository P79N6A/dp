#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <time.h>
#include <pthread.h>
#include <poseidon_ors_model.pb.h>
#include "sys/stat.h"
#include "sys/types.h"
#include "fcntl.h"
#include "json/json.h"
#include "campaign_budget.h"
#include "../common/utility.h"
#include "poseidon_proto.h"
#include "poseidon_ors_offline.pb.h"
#include "log.h"
#include "func.h"
#include "proto_helper.h"
#include "openssl/md5.h"
#include "../common/http_boost.h"
#include "hiredis.h"
#include "types.h"
#include "budget_pdb_pacing.h"

#include "bp_attr.h"
#include "monitor_api.h"


using namespace std;
using namespace poseidon::ors_offline;

#define URL_LEN 1024

int init(const string &log4conf,
         const string &log4cat) {
    if (!LOG_INIT(log4conf, log4cat)) {
        fprintf(stderr, "LOG_INIT error [%s, %s]", log4conf.c_str(), log4cat.c_str());
        return -1;
    }
    return 0;
}

// Get campaign posted budget form real time calculation sever batched.
int getCampaignPostedBudgetBatch(const poseidon::ors_offline::BudgetPacingConfig &config, map<UInt32, float> &campaign_posted_budgets) {
    LOG_INFO("getCampaignPostedBudgetBatch beginning.");
    int ret = 0;
    string respond;
    if (0 == httpGet(config.batch_campaign_server().host(), config.batch_campaign_server().port(),
                     config.batch_campaign_server().path(), respond)) {
        LOG_DEBUG("get batch campaign posted budget response:%s", respond.c_str());
        Json::Reader reader;
        Json::Value value;
        if (reader.parse(respond.c_str(), value)) {
            string list_key("list");
            if (!value.isMember(list_key.c_str())) {
                LOG_ERROR("Http batch respond has no campaign list infos.");
                //MON_ADD(ATTR_BUDGETPACING_BATCH_API_ERROR, 1);
                ret = -1;
            } else {
                Json::Value campaign_list = value[list_key];
                for (Json::Value::const_iterator it = campaign_list.begin(); it != campaign_list.end(); ++it) {
                    LOG_DEBUG("Handle json object:%s", it->toStyledString().c_str());
                    string campaign_id_key("campaign_id");
                    if (!it->isMember(campaign_id_key.c_str())) {
                        LOG_ERROR("This campaign json object has no %s.", campaign_id_key.c_str());
                        continue;
                    }
                    string campaign_daily_cost_key("campaign_daily_cost");
                    if (!it->isMember(campaign_daily_cost_key.c_str())) {
                        LOG_ERROR("This campaign json object has no %s.", campaign_daily_cost_key.c_str());
                        continue;
                    }
                    UInt32 campaign_id = (UInt32) strtoul(it->get(campaign_id_key, "").asString().c_str(), NULL, 10);
                    int campaign_daily_cost = it->get(campaign_daily_cost_key, "").asInt();
                    LOG_DEBUG("Parsed: campaign_id=%d\tcampaign_daily_cost=%d", campaign_id, campaign_daily_cost);

                    if (campaign_posted_budgets.find(campaign_id) != campaign_posted_budgets.end()) {
                        LOG_ERROR("Deped campaign id in batch http. campaign_id=%d.", campaign_id);
                        continue;
                    }
                    campaign_posted_budgets[campaign_id] = campaign_daily_cost;
                }

            }
        } else {
            LOG_ERROR("Batch respond json parse error, json_str is %s.", respond.c_str());
            //MON_ADD(ATTR_BUDGETPACING_BATCH_API_ERROR, 1);
            ret = -1;
        }
    } else {
        LOG_ERROR("Http request error(batch)!");
        //MON_ADD(ATTR_BUDGETPACING_BATCH_API_ERROR, 1);
        ret = -1;
    }
    LOG_INFO("getCampaignPostedBudgetBatch ended successfully.");
    return ret;
}

// Get campaign posted budget from real time calculation sever per campaign id.
int getCampaignPostedBudget(const poseidon::ors_offline::BudgetPacingConfig &config,
                            const map<UInt32, float> &campaign_posted_budgets,
                            UInt32 campaign_id) {
    LOG_INFO("getCampaignPostedBudget beginning.");
    map<UInt32, float>::const_iterator map_it = campaign_posted_budgets.find(campaign_id);
    int campaign_posted_budget = 0;
    if (map_it != campaign_posted_budgets.end()) {
        LOG_INFO("Campaign id(%d) found in campaign_posted_budgets.", campaign_id);
        campaign_posted_budget = map_it->second;
    } else {
        LOG_INFO("Campaign id(%d) do not in campaign_posted_budgets.", campaign_id);

        char path[URL_LEN];
        sprintf(path, "%s%d", config.per_campaign_server().path().c_str(), campaign_id);
        string respond;
        if (0 == httpGet(config.per_campaign_server().host(), config.per_campaign_server().port(), path, respond)) {
            Json::Reader reader;
            Json::Value value;
            if (reader.parse(respond.c_str(), value)) {
                campaign_posted_budget = value["campaign_daily_cost"].asInt();
                LOG_DEBUG("campaign_posted_budget=%d.", campaign_posted_budget);
            } else {
                campaign_posted_budget = -1;
                LOG_ERROR("Json string parse error. Json=%s.", respond.c_str());
            }
        } else {
            campaign_posted_budget = -1;
            LOG_ERROR("Http per campaign request error.");
        }
    }

    LOG_INFO("getCampaignPostedBudget end.");
    return campaign_posted_budget;
}

void addBudgetPacingItem(poseidon::ors::BudgetPacingItem *item,
                         UInt32 campaign_id,
                         float bidding_rate,
                         float budget_exceeding_ratio,
                         int bidding_mode,
                         UInt32 target_adx,
                         float fixed_price) {
    item->set_campaign_id(campaign_id);
    item->set_budget_pacing_ratio(bidding_rate);
    item->set_budget_exceeding_ratio(budget_exceeding_ratio);
    item->set_bidding_mode(bidding_mode);
    item->set_adx_id(target_adx);
    if (bidding_mode == ADS_FIXED_BIDDING) {
        item->set_fixed_price(fixed_price);
    }
}

// Parse campaign budget from ads index file.
int getCampaignBudgets(const string &file_path,
                       vector<poseidon::ors_offline::CampaignBudget> &campaign_budgets,
                       const time_t &t,
                       int compaign_type) {
    LOG_INFO("getCampaignBudgets beginning.");
    ifstream in(file_path.c_str(), ios::in | ios::binary);
    if (!in) {
        LOG_ERROR("Failed to open file %s", file_path.c_str());
        return -1;
    }
    set<int> campaign_set;
    string line;
    Json::Reader reader;
    while (getline(in, line)) {
        Json::Value value;
        if (reader.parse(line, value)) {

            string type("compaign_type");
            if (value.isMember(type) && value.get(type, "1").asInt() != compaign_type) {
                LOG_INFO("ads info compaign type is RTB");
                continue;
            }

            poseidon::ors_offline::CampaignBudget campaign_budget;
            string campaign_id_key("campaign_id");
            if (!value.isMember(campaign_id_key.c_str())) {
                LOG_ERROR("Ads info do not have campaign_id. %s", line.c_str());
                continue;
            }
            int campaign_id = value["campaign_id"].asUInt();
            if (campaign_set.find(campaign_id) != campaign_set.end()) {
                LOG_WARN("Duped campaign_id=%d, %s", campaign_id, line.c_str());
                continue;
            }
            campaign_budget.set_campaign_id(campaign_id);

            string campaign_daily_budget_key("campaign_daily_budget");
            if (!value.isMember(campaign_daily_budget_key.c_str())) {
                LOG_ERROR("Ads info do not have campaign_daily_budget. %s", line.c_str());
                continue;
            }
            campaign_budget.set_budget(value[campaign_daily_budget_key.c_str()].asUInt64());

            string ad_time_type_key("ad_time_type");
            if (!value.isMember(ad_time_type_key.c_str())) {
                LOG_ERROR("Ads info do not have ad_time_type. %s", line.c_str());
                continue;
            }
            campaign_budget.set_ad_time_type(value[ad_time_type_key.c_str()].asUInt());

            if (campaign_budget.get_ad_time_type() == ::poseidon::ors_offline::SELECT_HALF_HOURS)//2)
            {
                string post_hours_key("post_hours");
                if (!value.isMember(post_hours_key.c_str())) {
                    LOG_ERROR("Ads info do not have post_hours. %s", line.c_str());
                    continue;
                }
                campaign_budget.set_post_hours(value[post_hours_key.c_str()].asString(), get_week_day(t));
            }

            string target_adx_key("source");
            if (!value.isMember(target_adx_key.c_str())) {
                LOG_ERROR("Ads info do not have target_adx(source). %s", line.c_str());
                continue;
            }
            campaign_budget.set_target_adx(value[target_adx_key.c_str()].asUInt());

            campaign_budgets.push_back(campaign_budget);
            campaign_set.insert(campaign_id);
        } else {
            LOG_ERROR("Ads info Json parse error, %s", line.c_str());
            continue;
        }
    }
    in.close();

    LOG_INFO("getCampaignBudgets end successfully.");
    return 0;
}

float getPacingChangeRate(double schedule_budget, double posted_budget) {
    float pacing_change_rate = 0.0f;
    if (schedule_budget > posted_budget) {
        if (posted_budget == 0.0f) {
            pacing_change_rate = 1.0f;
        } else {
            pacing_change_rate = (schedule_budget - posted_budget) / posted_budget;
            if (pacing_change_rate > 1.0f) {
                pacing_change_rate = 1.0f;
            }
        }
    } else if (schedule_budget < posted_budget) {
        if (schedule_budget == 0.0f) {
            pacing_change_rate = 0;
        } else {
            pacing_change_rate = (schedule_budget - posted_budget) / schedule_budget;
            if (pacing_change_rate < -0.5f) {
                pacing_change_rate = -0.5;
            }
        }
    }
    LOG_DEBUG("Pacing change rate is:%f, scheduled budget is:%f, posted budget is:%f.", pacing_change_rate,
              schedule_budget, posted_budget);

    return pacing_change_rate;
}

// 从redis里去无符号整形值
// return -1, 如果没有取到
int getUIntFromRedis(string redis_ip, int redis_port, string redis_cmd) {
    LOG_INFO("getUIntFromRedis beginning.");
    redisContext *c;
    struct timeval timeout = {1, 500000};
    c = redisConnectWithTimeout(redis_ip.c_str(), redis_port, timeout);
    if (c == NULL || c->err) {
        if (c) {
            LOG_ERROR("Connection error: %s. redis_server_ip=%s, redis_server_port=%d.", c->errstr, redis_ip.c_str(), redis_port);
        } else {
            LOG_ERROR("Connection error: can't allocate redis context. redis_server_ip=%s, redis_server_port=%d.", redis_ip.c_str(), redis_port);
        }
        //MON_ADD(ATTR_BUDGETPACING_REDIS_ACCESS_ERROR, 1);
        return -1;
    }

    redisReply *reply = (redisReply *) redisCommand(c, redis_cmd.c_str());
    LOG_DEBUG("Execute redis command:%s.", redis_cmd.c_str());
    if (reply == NULL) {
        LOG_ERROR("Cannot get realtime QPS from redis.");
        redisFree(c);
        //MON_ADD(ATTR_BUDGETPACING_REDIS_ACCESS_ERROR, 1);
        return -1;
    }
    if (reply->str == NULL) {
        LOG_ERROR("Redis reply string is NULL.");
        freeReplyObject(reply);
        redisFree(c);
        //MON_ADD(ATTR_BUDGETPACING_REDIS_ACCESS_ERROR, 1);
        return -1;
    }

    int value = atoi(reply->str);

    freeReplyObject(reply);
    redisFree(c);
    LOG_INFO("getUIntFromRedis ended successfully.");
    return value;
}

// 获取实时ORS QPS
// 返回 -1， 如果没有取到值
int getRealtimeQPS(const poseidon::ors_offline::BudgetPacingConfig &config,
                   const CampaignBudget &campaign_budget,
                   time_t t) {
    LOG_INFO("getRealTimeQPS beginning.");
    char full_t[13];
    strftime(full_t, 13, "%Y%m%d%H%M", localtime(&t));
    stringstream ss;
    ss << campaign_budget.get_target_adx();
    string target_adx = ss.str();

    string realtime_qps_cmd("GET FS_ADX_" + target_adx + "_RT_1_" + full_t);

    int realtime_qps = getUIntFromRedis(config.redis_server().ip(), config.redis_server().port(), realtime_qps_cmd);
    LOG_DEBUG("ORS QPS at %s is:%d.", full_t, realtime_qps);
    LOG_INFO("getRealTimeQPS ended successfully.");

    return -1;
}

// 获取QPS调整率，大于等于1
float getQPSAdjustRate(const poseidon::ors_offline::BudgetPacingConfig &config,
                       const CampaignBudget &campaign_budget,
                       time_t t) {
    LOG_INFO("getQPSAdjustRate beginning.");

    stringstream ss;
    ss << campaign_budget.get_target_adx();
    string target_adx = ss.str();

    char min_now[13];
    strftime(min_now, 13, "%Y%m%d%H%M", localtime(&t));

    string min_now_qps_cmd("GET FS_ADX_" + target_adx + "_AVG_1_" + min_now);
    int min_now_qps = getUIntFromRedis(config.redis_server().ip(), config.redis_server().port(), min_now_qps_cmd);
    if (min_now_qps <= 0) {
        LOG_DEBUG("The QPS of %s is 0 or do not have(%d).", min_now, min_now_qps);
        return 1.0f;
    }

    char min_last[13];
    time_t t_last = t - 60;
    strftime(min_last, 13, "%Y%m%d%H%M", localtime(&t_last));
    string min_last_qps_cmd("GET FS_ADX_" + target_adx + "_AVG_1_" + min_last);
    int min_last_qps = getUIntFromRedis(config.redis_server().ip(), config.redis_server().port(), min_last_qps_cmd);
    if (min_last_qps <= 0) {
        LOG_DEBUG("The QPS of %s is 0 or do not have(%d).", min_last, min_last_qps);
        return 1.0f;
    }

    if (min_last_qps >= min_now_qps) {
        LOG_DEBUG("The [QPS(%s)=%d] <= [QPS(%s)=%d]", min_now, min_now_qps, min_last, min_last_qps);
        return 1.0f;
    }

    float qps_adjust_rate = (float) min_now_qps / (float) min_last_qps;
    if (qps_adjust_rate > config.model_params().max_qps_adjust_rate()) {
        qps_adjust_rate = config.model_params().max_qps_adjust_rate();
        LOG_DEBUG("Rescale qps_adjust_rate to max_qps_adjust_rate=%f.", config.model_params().max_qps_adjust_rate());
    }

    LOG_DEBUG("qps_adjust_rate=%f, QPS(%s)=%d, QPS(%s)=%d", qps_adjust_rate, min_now, min_now_qps, min_last,
              min_last_qps);
    LOG_INFO("getQPSAdjustRate ended successfully.");
    //return qps_adjust_rate;
    return 1.0f;
}

// cost=(left_budget/left_minute)*(cur_hour_percent/(sum_left_hour_percent/left_hour_count))
// 此函数计算后面部分(cur_hour_percent/(sum_left_hour_percent/left_hour_count))
// 默认返回1.0f
float minsBudgetAdjustPerHourlyBudgetPercent(const string &budget_hourly_percent_file,
                                             time_t t,
                                             int adx_id) {
    LOG_INFO("minsBudgetAdjustPerHourlyBudgetPercent beginning.");
    float mins_budget_adjust = 1.0f;
    ifstream in(budget_hourly_percent_file.c_str(), ios::in | ios::binary);
    if (!in) {
        LOG_ERROR("Failed to open file:%s.", budget_hourly_percent_file.c_str());
        return mins_budget_adjust;
    }
    string line;
    map<int, map<int, float> > budget_hourly_percent;
    while (getline(in, line)) {
        vector<string> parts = split(line, "\t");
        if (parts.size() != 3) {
            LOG_ERROR("Invalid line={%s} in budget_hourly_percent_file:%s", line.c_str(),
                      budget_hourly_percent_file.c_str());
            continue;
        }
        int adx_id_tmp = atoi(parts[0].c_str());
        int hour_tmp = atoi(parts[1].c_str());
        float percent_tmp = atof(parts[2].c_str());
        map<int, map<int, float> >::iterator adx_it = budget_hourly_percent.find(adx_id_tmp);
        if (adx_it == budget_hourly_percent.end()) {
            map<int, float> hourly_percent_tmp;
            hourly_percent_tmp[hour_tmp] = percent_tmp;
            budget_hourly_percent[adx_id_tmp] = hourly_percent_tmp;
        } else {
            budget_hourly_percent[adx_id_tmp][hour_tmp] = percent_tmp;
        }
    }
    in.close();

    map<int, map<int, float> >::iterator adx_it = budget_hourly_percent.find(adx_id);
    if (adx_it == budget_hourly_percent.end()) {
        LOG_ERROR("Cannot find adx(adx_id=%d) in file:%s.", adx_id, budget_hourly_percent_file.c_str());
        return mins_budget_adjust;
    }
    string hour_str = getH(t);
    int cur_hour = atoi(hour_str.c_str());
    float cur_hour_percent = adx_it->second[cur_hour];
    float sum_left_hour_percent = 0.0f;
    int left_hour_count = 24 - cur_hour;
    for (int i = cur_hour; i < 24; i++) {
        sum_left_hour_percent += adx_it->second[i];
    }
    mins_budget_adjust = cur_hour_percent / (sum_left_hour_percent / left_hour_count);
    LOG_DEBUG("cur_hour=%d; cur_hour_percent=%f; sum_left_hour_percent=%f; left_hour_count=%d; mins_budget_adjust=%f.",
              cur_hour, cur_hour_percent, sum_left_hour_percent, left_hour_count, mins_budget_adjust);

    LOG_INFO("minsBudgetAdjustPerHourlyBudgetPercent ended successfully.");
    return mins_budget_adjust;
}

float getForecastPacingChangeRate(const poseidon::ors_offline::BudgetPacingConfig &config,
                                  map<UInt32, int> &campaign_posted_budget,
                                  map<UInt32, float> &history_posted_budget_smooth,
                                  poseidon::ors_offline::CampaignBudget &campaign_budget,
                                  time_t t,
                                  int posted_budget) {
    LOG_INFO("getForecastPacingChangeRate beginning.");

    float pacing_change_rate = 0.0f;

    map<UInt32, int>::iterator it = campaign_posted_budget.find(campaign_budget.get_campaign_id());
    if (it == campaign_posted_budget.end()) {
        pacing_change_rate = 0.0f;
        campaign_posted_budget[campaign_budget.get_campaign_id()] = posted_budget;
        LOG_DEBUG("There is no accumulate posted budget for last minute. ==> pacing_change_rate=%f",
                  pacing_change_rate);
        return pacing_change_rate;
    }

    int last_acc_posted_budget = it->second;
    float last_mins_posted_budget = (float) (posted_budget - last_acc_posted_budget);

    map<UInt32, float>::iterator his_it = history_posted_budget_smooth.find(campaign_budget.get_campaign_id());
    if (his_it != history_posted_budget_smooth.end()) {
        last_mins_posted_budget = 0.5 * last_mins_posted_budget + 0.5 * his_it->second;
        LOG_DEBUG(
                "Smoothing last minute posted budget:last_mins_posted_budget_smoothed=%f, last_mins_posted_budget=%f, history_posted_budget=%f.",
                last_mins_posted_budget, (float) (posted_budget - last_acc_posted_budget), his_it->second);
    } else {
        LOG_DEBUG("Has no smoothed history posted budget data, last_mins_posted_budget=%f", last_mins_posted_budget);
    }

    campaign_posted_budget[campaign_budget.get_campaign_id()] = posted_budget;
    history_posted_budget_smooth[campaign_budget.get_campaign_id()] = last_mins_posted_budget;

    if (last_mins_posted_budget < 0) {
        pacing_change_rate = 0.0f;
        LOG_ERROR(
                "last_mins_posted_budget < 0: last_mins_posted_budget=%f, last_acc_posted_budget=%d, posted_budget=%d ==>pacing_change_rate=%f.",
                last_mins_posted_budget, last_acc_posted_budget, posted_budget, pacing_change_rate);
        campaign_posted_budget.erase(campaign_budget.get_campaign_id());
        history_posted_budget_smooth.erase(campaign_budget.get_campaign_id());
        return pacing_change_rate;
    } else if (last_mins_posted_budget == 0) {
        pacing_change_rate = 1.0f;
        LOG_DEBUG(
                "last_mins_posted_budget == 0: last_mins_posted_budget=%f, last_acc_posted_budget=%d, posted_budget=%d ==> pacing_change_rate=%f.",
                last_mins_posted_budget, last_acc_posted_budget, posted_budget, pacing_change_rate);
        return pacing_change_rate;
    }

    int total_posted_time = campaign_budget.get_total_posted_time(get_mins_of_day(t));
    int total_schedule_time = campaign_budget.get_total_schedule_time();
    int remaining_time = total_schedule_time - total_posted_time;
    if (remaining_time <= 0) {
        pacing_change_rate = 0.0f;
        LOG_DEBUG(
                "remaining_time <= 0: remaining_time=%d, total_posted_time=%d, total_schedule_time=%d ==> pacing_change_rate=%f",
                remaining_time, total_posted_time, total_schedule_time, pacing_change_rate);
        return pacing_change_rate;
    }


    int total_budget = campaign_budget.get_budget();
    int remaining_budget = total_budget - posted_budget;
    if (remaining_budget <= 0) {
        pacing_change_rate = 0.0f;
        LOG_DEBUG(
                "remaining_budget <= 0: remaining_budget=%d, total_budget=%d, posted_budget=%d ==> pacing_change_rate=%f",
                remaining_budget, total_budget, posted_budget, pacing_change_rate);
        return pacing_change_rate;
    }

    float mins_budget_adjust = minsBudgetAdjustPerHourlyBudgetPercent(
            config.file_manager().budget_hourly_percent_file(),
            t,
            campaign_budget.get_target_adx());

    float mins_schedule_budget = (float) (remaining_budget) / (float) remaining_time;

    LOG_DEBUG( "Calculate mins_schedule_budget based on remaining budget and time:mins_schedule_budget=%f, remaining_budget=%d, remaining_time=%d.", mins_schedule_budget, remaining_budget, remaining_time);

    mins_schedule_budget *= mins_budget_adjust;

    LOG_DEBUG(
            "Adjust mins_schedule_budget based on hourly budget percent, before(=%f), after(=%f). mins_budget_adjust=%f",
            (float) (remaining_budget) / (float) remaining_time, mins_schedule_budget, mins_budget_adjust);

    if (mins_schedule_budget > last_mins_posted_budget) {
        pacing_change_rate = (mins_schedule_budget - last_mins_posted_budget) / last_mins_posted_budget;
        LOG_DEBUG("pacing_change_rate=%f, mins_schedule_budget=%f, last_mins_posted_budget=%f", pacing_change_rate,
                  mins_schedule_budget, last_mins_posted_budget);
        if (pacing_change_rate > 1.0f) {
            LOG_DEBUG("Adjust pacing_change_rate to 1.0f");
            pacing_change_rate = 1.0f;
        }
    } else if (mins_schedule_budget < last_mins_posted_budget) {
        pacing_change_rate = (mins_schedule_budget - last_mins_posted_budget) / mins_schedule_budget;
        LOG_DEBUG("pacing_change_rate=%f, mins_schedule_budget=%f, last_mins_posted_budget=%f", pacing_change_rate,
                  mins_schedule_budget, last_mins_posted_budget);
        if (pacing_change_rate < -0.5f) {
            LOG_DEBUG("Adjust pacing_change_rate to -0.5f");
            pacing_change_rate = -0.5f;
        }
    }
    LOG_INFO("getForecastPacingChangeRate ended successfully(pacing_change_rate=%f).", pacing_change_rate);
    return pacing_change_rate;
}


// 根据adx从配置中查找bidding_mode
// 如果查找不到对应adx的bidding_mode，则返回正常出价模式
int getBiddingMode(const poseidon::ors_offline::BudgetPacingConfig &config,
                   UInt32 adx,
                   UInt32 campaign_id) {
    int bidding_mode = ADS_NORMAL_BIDDING;
    for (int i = 0; i < config.model_params().adx_bidding_mode_size(); i++) {
        if (adx == config.model_params().adx_bidding_mode(i).adx_id()) {
            bidding_mode = config.model_params().adx_bidding_mode(i).bidding_mode();
        }
    }

    for (int i = 0; i < config.model_params().campaign_bidding_mode_size(); i++) {
        if (campaign_id == config.model_params().campaign_bidding_mode(i).campaign_id()) {
            bidding_mode = config.model_params().campaign_bidding_mode(i).bidding_mode();
        }
    }

    return bidding_mode;
}

float getFixedBiddingPrice(const poseidon::ors_offline::BudgetPacingConfig &config,
                           UInt32 adx,
                           UInt32 campaign_id) {
    float fixed_price = -1;
    for (int i = 0; i < config.model_params().adx_bidding_mode_size(); i++) {
        if (adx == config.model_params().adx_bidding_mode(i).adx_id()) {
            fixed_price = config.model_params().adx_bidding_mode(i).fixed_price();
        }
    }

    for (int i = 0; i < config.model_params().campaign_bidding_mode_size(); i++) {
        if (campaign_id == config.model_params().campaign_bidding_mode(i).campaign_id()) {
            fixed_price = config.model_params().campaign_bidding_mode(i).fixed_price();
        }
    }

    return fixed_price;
}

float getFixedBiddingRate(const poseidon::ors_offline::BudgetPacingConfig &config,
                          UInt32 adx,
                          UInt32 campaign_id) {
    float fixed_bidding_rate = -1;
    for (int i = 0; i < config.model_params().adx_bidding_mode_size(); i++) {
        if (adx == config.model_params().adx_bidding_mode(i).adx_id()) {
            fixed_bidding_rate = config.model_params().adx_bidding_mode(i).bidding_rate();
        }
    }

    for (int i = 0; i < config.model_params().campaign_bidding_mode_size(); i++) {
        if (campaign_id == config.model_params().campaign_bidding_mode(i).campaign_id()) {
            fixed_bidding_rate = config.model_params().campaign_bidding_mode(i).bidding_rate();
        }
    }

    return fixed_bidding_rate;
}

void monitor_for_budget_pacing(
        const poseidon::ors_offline::BudgetPacingConfig &config,
        CampaignBudget &campaign_budget,
        float bidding_rate,
        float budget_exceeding_ratio,
        int posted_budget,
        time_t t,
        map<UInt32, float> &hourly_posted_budget){
    LOG_INFO("monitor function begins....");

    if(bidding_rate > 0 && budget_exceeding_ratio >= 1.0f){
        LOG_ERROR("budget pacing is over the total budget, but bidding_rate is bigger than 0.0f");
        //MON_ADD(ATTR_BUDGETPACING_BIDDING_RATE_ERROR, 1);
    }

    //init hour config file
    ifstream in( config.file_manager().budget_hourly_percent_file().c_str(), ios::in | ios::binary );
    if (!in) {
        LOG_ERROR("Failed to open file:%s.", config.file_manager().budget_hourly_percent_file().c_str());
        //MON_ADD(ATTR_BUDGETPACING_BIDDING_RATE_ERROR, 1);
    }

    string line;
    map<int, map<int, float> > budget_hourly_percent;
    while (getline(in, line)) {
        vector<string> parts = split(line, "\t");
        if (parts.size() != 3) {
            LOG_ERROR("Invalid line={%s} in budget_hourly_percent_file:%s", line.c_str(), config.file_manager().budget_hourly_percent_file().c_str());
            continue;
        }
        int adx_id_tmp = atoi(parts[0].c_str());
        int hour_tmp = atoi(parts[1].c_str());
        float percent_tmp = atof(parts[2].c_str());
        map<int, map<int, float> >::iterator adx_it = budget_hourly_percent.find(adx_id_tmp);
        if (adx_it == budget_hourly_percent.end()) {
            map<int, float> hourly_percent_tmp;
            hourly_percent_tmp[hour_tmp] = percent_tmp;
            budget_hourly_percent[adx_id_tmp] = hourly_percent_tmp;
        } else {
            budget_hourly_percent[adx_id_tmp][hour_tmp] = percent_tmp;
        }
    }
    in.close();

    int total_posted_time = campaign_budget.get_total_posted_time(get_mins_of_day(t));
    int total_schedule_time = campaign_budget.get_total_schedule_time();
    int remaining_time = total_schedule_time - total_posted_time;
    int total_budget = campaign_budget.get_budget();
    double schedule_budget = campaign_budget.get_schedule_budget(get_mins_of_day(t));

    map<int, map<int, float> >::iterator adx_it = budget_hourly_percent.find(campaign_budget.get_target_adx());
    if (adx_it == budget_hourly_percent.end()) {
        LOG_ERROR("Cannot find adx(adx_id=%d) in file:%s.", campaign_budget.get_target_adx(), config.file_manager().budget_hourly_percent_file().c_str());
    }else{
        string hour_str = getH(t);
        int cur_hour = atoi(hour_str.c_str());
        float cur_hour_percent = adx_it->second[cur_hour];
        float sum_past_hour_percent = 0.0f;
        int past_hour_count = cur_hour - 1;
        for (int i = 0; i < cur_hour; i++) {
            sum_past_hour_percent += adx_it->second[i];
        }

        //monitor per hour
        if ( total_posted_time % 60 == 0 && total_posted_time != 0){

            //monitor by schedule budget
            if ( schedule_budget / total_budget > 0.5 ){
                if ( posted_budget < schedule_budget && budget_exceeding_ratio < sum_past_hour_percent ){
                    LOG_ERROR("half total time past, campaign id: %d doesn't not arrive schedule_budget %f as well as sum_past_hour_percent about last hour: %f, now the total posted percent is %f", campaign_budget.get_campaign_id(), schedule_budget, sum_past_hour_percent, budget_exceeding_ratio);
                    //MON_ADD(ATTR_BUDGETPACING_BIDDING_RATE_ERROR, 1);
                }

                if ( posted_budget > schedule_budget && budget_exceeding_ratio > sum_past_hour_percent ){
                    LOG_ERROR("half total time past, campaign id: %d doesn't not arrive schedule_budget %f as well as sum_past_hour_percent about last hour: %f, now the total posted percent is %f", campaign_budget.get_campaign_id(), schedule_budget, sum_past_hour_percent, posted_budget / total_budget);
                    //MON_ADD(ATTR_BUDGETPACING_BIDDING_RATE_ERROR, 1);
                }
            }

            //monitor by left budget
            int left_budget = total_budget - posted_budget;
            float future_avg = (float) left_budget / remaining_time;
            float past_avg = posted_budget / total_posted_time;
            float percent = (1 - sum_past_hour_percent) / sum_past_hour_percent;
            if ( future_avg / past_avg > 2 * percent ){
                if ( bidding_rate != 1){
                    //MON_ADD(ATTR_BUDGETPACING_BIDDING_RATE_ERROR, 1);
                    LOG_ERROR("campaign id: %d, left budget %d, left_minutes: %d, future_avg is %f, past budget %d, past_avg is %f, bidding_rate is %s",  campaign_budget.get_campaign_id(), left_budget, remaining_time, future_avg, posted_budget,past_avg,bidding_rate);
                }
            }

            if ( future_avg / past_avg < 0.5 * percent ){
                //MON_ADD(ATTR_BUDGETPACING_BIDDING_RATE_ERROR, 1);
                LOG_ERROR("campaign id: %d, left budget %d, left_minutes: %d, future_avg is %f, past budget %d, past_avg is %f, bidding_rate is %s", campaign_budget.get_campaign_id(), left_budget, remaining_time, future_avg, posted_budget,past_avg,bidding_rate);
            }
        }
    }

    //monitor per hour
    if ( total_posted_time % 60 == 0 && total_posted_time != 0){
        double past_hour_schedule_budget = schedule_budget - campaign_budget.get_schedule_budget(get_mins_of_day(t - 60));
        map<UInt32, float>::iterator it_hourly = hourly_posted_budget.find(campaign_budget.get_campaign_id());
        if(it_hourly != hourly_posted_budget.end()){
            double hourly_posted = posted_budget - it_hourly->second;
            if ( hourly_posted < past_hour_schedule_budget){
                //MON_ADD(ATTR_BUDGETPACING_BIDDING_RATE_ERROR, 1);
                LOG_ERROR("campaign id: %d last hourly posted budget is %f, less then schedule budget: %f ", campaign_budget.get_campaign_id(), hourly_posted, schedule_budget);
            }

            it_hourly->second = posted_budget;
        }else{
            hourly_posted_budget[campaign_budget.get_campaign_id()] = posted_budget;
        }
    }

    LOG_INFO("monitor function ends....");
}
/**
 *
 * @param config
 * @param campaign_bidding_rate
 * @param campaign_posted_budget
 * @param history_posted_budget_smooth
 * @param campaign_budgets
 * @param campaign_posted_budgets
 * @param t
 * @param budget_pacing_model
 * @return
 */
int updateBudgetPacingModel(const poseidon::ors_offline::BudgetPacingConfig &config,
                            map<UInt32, float> &campaign_bidding_rate,
                            map<UInt32, int> &campaign_posted_budget,
                            map<UInt32, float> &history_posted_budget_smooth,
                            vector<poseidon::ors_offline::CampaignBudget> &campaign_budgets,
                            const map<UInt32, float> &campaign_posted_budgets,
                            time_t t,
                            poseidon::ors::BudgetPacingModel &budget_pacing_model,
                            map<UInt32, float> &hourly_posted_budget) {
    LOG_INFO("updateBudgetPacingModel beginning.");
    for (vector<poseidon::ors_offline::CampaignBudget>::iterator it = campaign_budgets.begin();
         it != campaign_budgets.end(); ++it) {
        double schedule_budget = it->get_schedule_budget(get_mins_of_day(t));
        int posted_budget = getCampaignPostedBudget(config, campaign_posted_budgets, it->get_campaign_id());
        if (posted_budget < 0) {
            continue;
        }

        float bidding_rate = 0.0f;
        float budget_exceeding_ratio = 0.0f;
        int target_adx = it->get_target_adx();
        int bidding_mode = getBiddingMode(config, target_adx, it->get_campaign_id());
        float fixed_price = 0.0f;


        if (it->is_during_schedule_time(get_mins_of_day(t))) { // 此campaign投放中
            // 更新bidding_rate
            // 从map中根据campaign_id取bidding_rate, 若不存在次campaign_id，则初始化为配置值并插入到map中
            map<UInt32, float>::iterator map_it = campaign_bidding_rate.find(it->get_campaign_id());
            if (map_it == campaign_bidding_rate.end()) {
                bidding_rate = config.model_params().init_pacing_rate();
                campaign_bidding_rate.insert(pair<UInt32, float>(it->get_campaign_id(), bidding_rate));
            } else {
                bidding_rate = map_it->second;
            }

            if (bidding_mode == ADS_FIXED_BIDDING) { // 1 分钱出价模式: 设置bidding_rate为配置值
                fixed_price = getFixedBiddingPrice(config, target_adx, it->get_campaign_id());
                bidding_rate = getFixedBiddingRate(config, target_adx, it->get_campaign_id());
            } else { // 正常投放模式
                if (0 != getRealtimeQPS(config, *it, t)) { // 没有取到实时QPS或QPS>0，正常调整出价率，只有取得的QPS是0时才不调整竞价率
                    float pacing_change_rate = config.model_params().pacing_change_rate();
                    if (config.model_params().dynamic_pacing_change_rate() == 1) { // 动态调整bidding_rate的调整幅度
                        LOG_DEBUG("Using dynamic pacing change rate.");
                        // pacing_change_rate = getPacingChangeRate(schedule_budget, (double)posted_budget);
                        pacing_change_rate = getForecastPacingChangeRate(config, campaign_posted_budget,
                                                                         history_posted_budget_smooth, *it, t,
                                                                         posted_budget);
                        LOG_DEBUG(
                                "Dynamic adjust bidding_rate with pacing_change_rate(=%f), bidding_rate=(before=%f, after=%f)",
                                pacing_change_rate, bidding_rate, bidding_rate * (1 + pacing_change_rate));
                        bidding_rate *= (1 + pacing_change_rate);
                    } else { // 静态调整bidding_rate, 调整幅度通过配置文件设定
                        LOG_DEBUG("Using static pacing change rate.");
                        if ((double) posted_budget < schedule_budget) {
                            bidding_rate *= (1 + pacing_change_rate);
                        } else if ((double) posted_budget > schedule_budget) {
                            bidding_rate *= (1 - pacing_change_rate);
                        }
                    }

                    // qps_adjust_rate是一个大于等于1的数, 用于在QPS上升时对竞价率进行调整，避免预算消耗波动太大
                    float qps_adjust_rate = getQPSAdjustRate(config, *it, t);
                    if (qps_adjust_rate > 1.0f) {
                        LOG_DEBUG("Adjust bidding_rate with qps_adjust_rate=%f, bidding_rate=(before:%f, after:%f).",
                                  qps_adjust_rate, bidding_rate, bidding_rate / qps_adjust_rate);
                        bidding_rate /= qps_adjust_rate;
                    } else {
                        LOG_DEBUG("Do not adjust bidding_rate as qps_adjust_rate=%f.", qps_adjust_rate);
                    }

                    if (bidding_rate >= 1) {
                        LOG_DEBUG("Rescale bidding_rate to 1.0f");
                        bidding_rate = 1;
                    }
                }
            }

            // 计算budget_exceeding_ratio
            UInt64 total_schedule_budget = it->get_budget();
            if (schedule_budget > 0) {
                budget_exceeding_ratio = (float) posted_budget / (float) total_schedule_budget;
                if (budget_exceeding_ratio >= 1) {
                    budget_exceeding_ratio = 1;
                    bidding_rate = 0;
                }
            } else {
                budget_exceeding_ratio = 0;
                bidding_rate = 0;
            }

            // 更新bidding_rate到map中
            if (bidding_mode == ADS_FIXED_BIDDING) {
                campaign_bidding_rate[it->get_campaign_id()] = config.model_params().init_pacing_rate();
            } else {
                campaign_bidding_rate[it->get_campaign_id()] = bidding_rate;
            }

            if (it->get_budget() <= 0 || bidding_rate <= 0.0f) {
                map<UInt32, float>::iterator map_it = campaign_bidding_rate.find(it->get_campaign_id());
                if (map_it != campaign_bidding_rate.end()) {
                    campaign_bidding_rate.erase(map_it);
                }
            }
        } else { // 不在投放计划时间
            // 从map中去除该campaign_id
            map<UInt32, float>::iterator map_it = campaign_bidding_rate.find(it->get_campaign_id());
            if (map_it != campaign_bidding_rate.end()) {
                campaign_bidding_rate.erase(map_it);
            }

            //设置 bidding_rate 和 budget_exceeding_ratio 为0
            bidding_rate = 0;
            budget_exceeding_ratio = 0;
        }
        addBudgetPacingItem(budget_pacing_model.add_items(), it->get_campaign_id(), bidding_rate, budget_exceeding_ratio, bidding_mode, target_adx, fixed_price);

        // code for monitor
        if ( budget_exceeding_ratio >= 1.0f && bidding_rate > 0.0f ){
            //MON_ADD(ATTR_BUDGETPACING_BIDDING_RATE_ERROR, 1);
            LOG_ERROR("budget exceeding ratio %f exceeds 1.0, bidding rate %f should be 0.0", budget_exceeding_ratio, bidding_rate);
        }
        if ( it->is_during_schedule_time(get_mins_of_day(t)) && bidding_mode != ADS_FIXED_BIDDING && config.model_params().dynamic_pacing_change_rate() == 1 ){
            monitor_for_budget_pacing(config, *it, bidding_rate, budget_exceeding_ratio, posted_budget, t, hourly_posted_budget);
        }else{
            if(bidding_rate != 0 || budget_exceeding_ratio != 0){
                LOG_ERROR("not in schedule time, both bidding rate %f and budget_exceeding_ratio %f should be zero", bidding_rate, budget_exceeding_ratio);
                //MON_ADD(ATTR_BUDGETPACING_BIDDING_RATE_ERROR, 1);
            }
        }

        LOG_DEBUG( "campaign_id=%d; daily_schedule_budget=%ul; schedule_budget=%f; posted_budget=%d; bidding_rate=%f; budget_exceeding_ratio=%f, bidding_mode=%d, target_adx=%d.", it->get_campaign_id(), it->get_budget(), schedule_budget, posted_budget, bidding_rate, budget_exceeding_ratio, bidding_mode, target_adx);

    }
    return 0;
}

int saveBudgetPacingModel(const poseidon::ors_offline::BudgetPacingConfig &config,
                          const poseidon::ors::BudgetPacingModel &budget_pacing_model) {
    LOG_INFO("saveBudgetPacingModel beginning.");
    fstream output(config.file_manager().results_pb_file().c_str(), ios::out | ios::trunc | ios::binary);
    if (!budget_pacing_model.SerializeToOstream(&output)) {
        LOG_ERROR("Failed write budget pacing model to file(%s).", config.file_manager().results_pb_file().c_str());
        return -1;
    }
    output.close();
    LOG_INFO("saveBudgetPacingModel end successfully.");
    return 0;
}

int pushData(const poseidon::ors_offline::BudgetPacingConfig &config,
             time_t t) {
    string pb_file(config.file_manager().results_pb_file());
    string pb_tag_file(config.file_manager().results_tag_file());
    string tag_gen_command = "md5sum " + pb_file + " > " + pb_tag_file;
    system(tag_gen_command.c_str());
    for (int i = 0; i < config.file_manager().remote_dir_size(); i++) {
        string push_command = "scp -P 9922 " + pb_file + " " + pb_tag_file + " " + config.file_manager().remote_dir(i);
        system(push_command.c_str());
        LOG_INFO("Pushed file(%s and %s) to path(%s).", pb_file.c_str(), pb_tag_file.c_str(),
                 config.file_manager().remote_dir(i).c_str());
    }

    if (!dir_exists(config.file_manager().results_history_path().c_str())) {
        LOG_ERROR("config.file_manager.results_history_path not exists:%s.",
                  config.file_manager().results_history_path().c_str());
        return -1;
    }

    string history_data_path(config.file_manager().results_history_path());
    char day[9];
    strftime(day, 9, "%Y%m%d", localtime(&t));
    char hour[3];
    strftime(hour, 3, "%H", localtime(&t));
    char mins[3];
    strftime(mins, 3, "%M", localtime(&t));
    char full_t[15];
    strftime(full_t, 15, "%Y%m%d%H%M%S", localtime(&t));

    history_data_path += string("/") + string(day);
    if (!dir_exists(history_data_path.c_str())) {
        if (0 != mkdir(history_data_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
            LOG_ERROR("Create dir failed:%s.", history_data_path.c_str());
            return -1;
        }
    }

    history_data_path += string("/") + string(hour);
    if (!dir_exists(history_data_path.c_str())) {
        if (0 != mkdir(history_data_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
            LOG_ERROR("Create dir failed:%s.", history_data_path.c_str());
            return -1;
        }
    }

    mins[1] = '\0';
    history_data_path += string("/") + string(mins);
    if (!dir_exists(history_data_path.c_str())) {
        if (0 != mkdir(history_data_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
            LOG_ERROR("Create dir failed:%s.", history_data_path.c_str());
            return -1;
        }
    }

    vector<string> pb_path_parts = split(pb_file, "/");
    string his_pb_file =
            history_data_path + string("/") + string(full_t) + string("_") + pb_path_parts[pb_path_parts.size() - 1];
    vector<string> pb_tag_path_parts = split(pb_tag_file, "/");
    string his_pb_tag_file = history_data_path + string("/") + string(full_t) + string("_") +
                             pb_tag_path_parts[pb_tag_path_parts.size() - 1];
    string cp_pb_command = string("cp ") + pb_file + " " + his_pb_file;
    string cp_pb_tag_command = "cp " + pb_tag_file + " " + his_pb_tag_file;
    system(cp_pb_command.c_str());
    LOG_INFO("Copy file from %s to %s.", pb_file.c_str(), his_pb_file.c_str());
    system(cp_pb_tag_command.c_str());
    LOG_INFO("Copy file from %s to %s.", pb_tag_file.c_str(), his_pb_tag_file.c_str());

    return 0;
}

int getAdsIndexFilePath(const string &md5_file_path, string &ads_index_file) {
    ifstream in(md5_file_path.c_str(), ios::in | ios::binary);
    if (!in) {
        LOG_ERROR("Failed to open md5 check file:%s", md5_file_path.c_str());
        return -1;
    }
    string line;
    if (!getline(in, line)) {
        LOG_ERROR("%s:file is empty!", md5_file_path.c_str());
        in.close();
        return -1;
    }
    in.close();

    vector<string> md5_file_parts = split(line, "  ");
    if (md5_file_parts.size() != 2) {
        LOG_ERROR("%s:file format error!", md5_file_path.c_str());
        return -1;
    }
    LOG_DEBUG("md5=%s\tfile_name=%s.", md5_file_parts[0].c_str(), md5_file_parts[1].c_str());
    vector<string> path_parts = split(md5_file_path, "/");
    ads_index_file = "";
    for (size_t i = 0; i < path_parts.size() - 1; ++i) {
        ads_index_file += path_parts[i] + "/";
    }
    ads_index_file += md5_file_parts[1];
    LOG_INFO("Ads index file:%s.", ads_index_file.c_str());

    string ads_index_file_md5;
    if (!poseidon::util::Func::BinaryFileMD5(ads_index_file.c_str(), &ads_index_file_md5)) {
        LOG_ERROR("Failed to generate ads index file md5.");
        return -1;
    }

    if (md5_file_parts[0] != ads_index_file_md5) {
        LOG_ERROR("Ads index file md5 error. Ads_index_file_md5={%s}, it expected to be {%s}",
                  ads_index_file_md5.c_str(), md5_file_parts[0].c_str());
        return -1;
    }
    LOG_INFO("Ads index file md5:{%s}", ads_index_file_md5.c_str());

    return 0;
}

int loadConfig(const char *config_path,
               poseidon::ors_offline::BudgetPacingConfig &config) {
    if (NULL == config_path) {
        fprintf(stderr, "Invalid config path.");
        return -1;
    }

    if (!poseidon::util::ParseProtoFromTextFormatFile(config_path, &config)) {
        fprintf(stderr, "Failed to parse budget pacing config.");
        return -1;
    }

    return 0;
}

int logConfig(const poseidon::ors_offline::BudgetPacingConfig &config) {
    if (config.has_log4config()
        && config.log4config().has_config_file()
        && config.log4config().has_category()) {
        LOG_INFO("Budget pacing config:config.log4config.config_file=%s", config.log4config().config_file().c_str());
        LOG_INFO("Budget pacing config:config.log4config.category=%s", config.log4config().category().c_str());
    } else {
        LOG_ERROR("Budget pacing config error:config.log4config");
        return -1;
    }

    if (config.has_model_params()
        && config.model_params().has_init_pacing_rate()
        && config.model_params().has_pacing_change_rate()
        && config.model_params().has_pacing_update_cycle_time()
        && config.model_params().has_resume_from_break_point()) {
        LOG_INFO("Budget pacing config:config.model_params.init_pacing_rate=%f",
                 config.model_params().init_pacing_rate());
        LOG_INFO("Budget pacing config:config.model_params.pacing_change_rate=%f",
                 config.model_params().pacing_change_rate());
        LOG_INFO("Budget pacing config:config.model_params.pacing_update_cycle_time=%d",
                 config.model_params().pacing_update_cycle_time());
        LOG_INFO("Budget pacing config:config.model_params.resume_from_break_point=%d",
                 config.model_params().resume_from_break_point());
    } else {
        LOG_ERROR("Budget pacing config error:config.model_params");
        return -1;
    }

    if (config.has_file_manager()
        && config.file_manager().has_ads_index_info_file()
        && config.file_manager().has_results_pb_file()
        && config.file_manager().has_results_tag_file()
        && config.file_manager().has_results_history_path()) {
        LOG_INFO("Budget pacing config:config.file_manager.ads_index_info_file=%s",
                 config.file_manager().ads_index_info_file().c_str());
        LOG_INFO("Budget pacing config:config.file_manager.results_pb_file=%s",
                 config.file_manager().results_pb_file().c_str());
        LOG_INFO("Budget pacing config:config.file_manager.results_tag_file=%s",
                 config.file_manager().results_tag_file().c_str());
        LOG_INFO("Budget pacing config:config.file_manager.results_history_path=%s",
                 config.file_manager().results_history_path().c_str());
        for (int i = 0; i < config.file_manager().remote_dir_size(); i++) {
            LOG_INFO("Budget pacing config:config.file_manager.remote_dir_%d=%s", i,
                     config.file_manager().remote_dir(i).c_str());
        }
    } else {
        LOG_ERROR("Budget pacing config error:config.file_manager");
        return -1;
    }

    if (config.has_batch_campaign_server()
        && config.batch_campaign_server().has_host()
        && config.batch_campaign_server().has_port()
        && config.batch_campaign_server().has_path()) {
        LOG_INFO("Budget pacing config:config.batch_campaign_server.host=%s",
                 config.batch_campaign_server().host().c_str());
        LOG_INFO("Budget pacing config:config.batch_campaign_server.port=%s",
                 config.batch_campaign_server().port().c_str());
        LOG_INFO("Budget pacing config:config.batch_campaign_server.path=%s",
                 config.batch_campaign_server().path().c_str());
    } else {
        LOG_ERROR("Budget pacing config error:config.batch_campaign_server");
        return -1;
    }

    if (config.has_per_campaign_server()
        && config.per_campaign_server().has_host()
        && config.per_campaign_server().has_port()
        && config.per_campaign_server().has_path()) {
        LOG_INFO("Budget pacing config:config.per_campaign_server.host=%s",
                 config.per_campaign_server().host().c_str());
        LOG_INFO("Budget pacing config:config.per_campaign_server.port=%s",
                 config.per_campaign_server().port().c_str());
        LOG_INFO("Budget pacing config:config.per_campaign_server.path=%s",
                 config.per_campaign_server().path().c_str());
    } else {
        LOG_ERROR("Budget pacing config error:config.per_campaign_server");
        return -1;
    }

    return 0;
}


int budgetPacingProcessor(/*const poseidon::ors_offline::BudgetPacingConfig& config,*/
        const char *config_path,
        map<UInt32, float> &campaign_bidding_rate,
        map<UInt32, int> &campaign_posted_budget,
        map<UInt32, float> &history_posted_budget_smooth,
        map<UInt32, float> &hourly_posted_budget
) {
    LOG_INFO("budgetPacingProcessor beginning!");

    time_t t = time(0);

    poseidon::ors_offline::BudgetPacingConfig config;
    if (-1 == loadConfig(config_path, config)) {
        fprintf(stderr, "Failed to parse budget pacing config.");
        return -1;
    }

    if (-1 == logConfig(config)) {
        LOG_ERROR("Budget pacing config error.");
        return -1;
    }

    string ads_index_file;
    if (-1 == getAdsIndexFilePath(config.file_manager().ads_index_info_file(), ads_index_file)) {
        LOG_ERROR("Failed to get ads index file path from file:%s",
                  config.file_manager().ads_index_info_file().c_str());
        return -1;
    }


    poseidon::ors::BudgetPacingModel budget_pacing_model;


    /**
     * logic for real time bidding (RTB)
     */
    int campaign_type = RTB;
    vector<poseidon::ors_offline::CampaignBudget> rtb_campaign_budgets;

    if (-1 == getCampaignBudgets(ads_index_file, rtb_campaign_budgets, t, campaign_type)) {
        LOG_ERROR("Failed to rtb get campaign budget from %s", ads_index_file.c_str());
        return -1;
    }

    /**
     * batch interface document for ansycing budgets depletion and  balance
     * http://doc.xxxx.ucweb.local/pages/viewpage.action?pageId=15172124
     */
    map<UInt32, float> campaign_posted_budgets;
    if (-1 == getCampaignPostedBudgetBatch(config, campaign_posted_budgets)) {
        LOG_ERROR("Failed to get batched campaign budgets");
    }

    if (0 !=
        updateBudgetPacingModel(config,
                                campaign_bidding_rate,
                                campaign_posted_budget,
                                history_posted_budget_smooth,
                                rtb_campaign_budgets,
                                campaign_posted_budgets,
                                t,
                                budget_pacing_model,
                                hourly_posted_budget)) {
        LOG_ERROR("Failed to update campaign budget pacing model.");
        return -1;
    }
    /**
     * logic for programmatic directly buy(PDB)
     */

    campaign_type = PDB;
    vector<poseidon::ors_offline::pdbCampaignBudget> pdb_campaign_budgets;
    map<string, set<UInt32> > mp_deal2campaign;

    if (-1 == getPDBDealBudgets(ads_index_file, pdb_campaign_budgets, t, campaign_type, mp_deal2campaign)) {
        LOG_ERROR("Failed to get pdb campaign budget from %s", ads_index_file.c_str());
        return -1;
    }


    if (-1 == updatePDBBudgetPacingModel(config, campaign_bidding_rate, campaign_posted_budget, history_posted_budget_smooth, pdb_campaign_budgets, t, budget_pacing_model, mp_deal2campaign)) {
        LOG_ERROR("Failed to update campaign budget pacing model.");
        return -1;
    }


    if (-1 == saveBudgetPacingModel(config, budget_pacing_model)) {
        LOG_ERROR("Failed to write budget pacing model");
        return -1;
    }

    if (-1 == pushData(config, t)) {
        return -1;
    }

    LOG_INFO("budgetPacingProcessor ended successfully.");
    return 0;
}

int loadPBFromBreakPoint(const string &pb_path, map<UInt32, float> &campaign_bidding_rate) {
    LOG_INFO("loadPBFromBreakPoint beginning.");
    fstream in(pb_path.c_str(), ios::in | ios::binary);
    if (!in) {
        LOG_ERROR("Failed to open file:%s.", pb_path.c_str());
        return -1;
    }

    poseidon::ors::BudgetPacingModel model;
    if (!model.ParseFromIstream(&in)) {
        LOG_ERROR("Failed to parse budget pacing model.");
        return -1;
    }
    for (int i = 0; i < model.items_size(); i++) {
        if (model.items(i).has_campaign_id()
            && model.items(i).has_budget_pacing_ratio()) {
            campaign_bidding_rate[model.items(i).campaign_id()] = model.items(i).budget_pacing_ratio();
        }
    }

    LOG_INFO("loadPBFromBreakPoint ended successfully.");
    return 0;
}

string setBudgetDate() {
    time_t t = time(0);
    char day[9];
    strftime(day, 9, "%Y%m%d", localtime(&t));
    string budget_date(day);

    LOG_INFO("Set budget date successfully:%s.", budget_date.c_str());
    return budget_date;
}

int initForNewDay(map<UInt32, float> &campaign_bidding_rate,
                  map<UInt32, int> &campaign_posted_budget,
                  map<UInt32, float> &history_posted_budget_smooth,
                  time_t t,
                  string &budget_date) {
    char day[9];
    strftime(day, 9, "%Y%m%d", localtime(&t));
    if (0 != strcmp(day, budget_date.c_str())) {
        campaign_bidding_rate.clear();
        campaign_posted_budget.clear();
        history_posted_budget_smooth.clear();
        LOG_INFO("Clear campaign bidding rate map, reset budget date(%s) to %s. size of campaign_bidding_rate is:%d.",
                 budget_date.c_str(), day, campaign_bidding_rate.size());
        budget_date = day;
        return 0;
    }

    return 1;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "argc=%d", argc);
        return 0;
    }

    poseidon::ors_offline::BudgetPacingConfig config;
    if (-1 == loadConfig(argv[1], config)) {
        fprintf(stderr, "Failed to parse budget pacing config.");
        return -1;
    }

    if (config.has_log4config()
        && config.log4config().has_config_file()
        && config.log4config().has_category()) {
        if (-1 == init(config.log4config().config_file(), config.log4config().category())) {
            fprintf(stderr, "Init failed.");
            return -1;
        }
    } else {
        fprintf(stderr, "Budget pacing config config.log4config error.");
        return -1;
    }
    LOG_INFO("Init successfully");

    if (-1 == logConfig(config)) {
        LOG_ERROR("Budget pacing config error.");
        return -1;
    }

    string budget_date(setBudgetDate());

    time_t time_interval = config.model_params().pacing_update_cycle_time();
    map<UInt32, float> campaign_bidding_rate;
    map<UInt32, int> campaign_posted_budget;
    map<UInt32, float> history_posted_budget_smooth;
    map<UInt32, float> hourly_posted_budget;

    if (config.model_params().resume_from_break_point() == 1) {
        if (-1 == loadPBFromBreakPoint(config.file_manager().results_pb_file(), campaign_bidding_rate)) {
            LOG_ERROR("Failed to load pb from break point.");
            return -1;
        }
    }


    while (true) {
        time_t start = time(0);
        LOG_INFO("Budget pacing processor start at %ld", start);

        if (1 == initForNewDay(campaign_bidding_rate, campaign_posted_budget, history_posted_budget_smooth, start,
                               budget_date)) {
            LOG_INFO("Budget date is:%s. size of campaign_bidding_rate is:%d.", budget_date.c_str(),
                     campaign_bidding_rate.size());
        }

        budgetPacingProcessor(/*config,*/ argv[1], campaign_bidding_rate, campaign_posted_budget,
                                          history_posted_budget_smooth, hourly_posted_budget);

        time_t end = time(0);
        LOG_INFO("Budget pacing processor end at %ld", end);

        time_t time_cost = end - start;
        LOG_INFO("Budget pacing processor takes %ld seconds", time_cost);

        //time_t time_sleep = time_interval - time_cost;
        time_t time_sleep = time_interval - (end % time_interval) + 1; // 每分钟的第1秒启动
        if (time_sleep > 0) {
            LOG_INFO("sleep %ld seconds.", time_sleep);
            sleep(time_sleep);
        }
    }

    return 0;
}
