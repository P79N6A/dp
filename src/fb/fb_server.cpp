/**
 **/

#include "fb_server.h"
#include <unistd.h>
#include <map>
#include "protocol/src/poseidon_proto.h"
#include "util/log.h"
#include "redis_access.h"
#include "hiredis.h"
#include <boost/algorithm/string.hpp>
#include "util/util_str.h"
#include "util/func.h"
#include "dmp_util.h"
#include "monitor_api.h"
#include "attr.h"
#include "config.h"
#include <stdio.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>

namespace poseidon
{
namespace feedback
{
namespace {
std::string MEM_FREQUENCY ="MEM_FREQUENCY:";
std::string GAMES_AD_COST_CAMP ="GAMES_AD_COST_CAMP:";
std::string GAMES_AD_COST_ADVER ="GAMES_AD_COST_ADVER:";
std::string GAMES_AD_COST_AD ="GAMES_AD_COST_AD:";
std::string GAMES_AD_COST_CREATIVE ="GAMES_AD_COST_CREATIVE:";
std::string MEM_KEY_ADVER ="ADVER:";
std::string MEM_KEY_CAMP = "CAMP:";
std::string MEM_KEY_AD = "AD:";
std::string MEM_KEY_CREATIVE = "CREATIVE:";
std::string	MEM_KEY_DEAL_ID = "DEAL_ID:";
std::string MEM_KEY_DEAL_CAMPAIGN_ID = "DEAL_CAMPAIGN_ID:";
const std::string MEM_STATISTICS_COM_DEF = ":";
}


/**
 * @brief               process req package
 **/
int FbServer::handle_read(const char * buf, const int len, struct sockaddr_in & client_addr)
{
    MON_ADD(ATTR_FB_REQ, 1);
    int rt=0;
    SessData * sess=NULL;
    do{
        sess = alloc_sess();
        LOG_INFO("init FBServer instance handle_read");
        if(sess == NULL )
        {
            LOG_ERROR("new SessData return NULL");
            rt=-1;
            break;
        }
        memcpy(&(sess->addr), &client_addr, sizeof(struct sockaddr_in) );
        FeedbackRequest & fbreq=sess->fbreq;
        FeedbackResponse & fbrsp=sess->fbrsp;
        if(!fbreq.ParseFromArray(buf, len))
        {
            MON_ADD(ATTR_FB_PARSE_ERR, 1);
            LOG_ERROR("new SessData return NULL");
            rt=-1;
            break;
        }
        LOG_DEBUG("pid[%d]fbreq[%s]", getpid(), fbreq.DebugString().c_str());
        std::string uid = sess->fbreq.dev_id();
        if(uid.empty() || uid.length()<=4) {
             uid = fbreq.acookie();
        }
        std::string datetime;
        util::Func::get_time_str(&datetime,"%Y%m%d");
//        time = "20161101";

        char yestDt[9];
		time_t now = time(NULL);
		struct tm *ts = localtime(&now);
		ts->tm_mday--;
		mktime(ts);
		strftime(yestDt, sizeof(yestDt), "%Y%m%d", ts);

        std::string ab = "";

        std::string adgroupCmd = "hmget "+GAMES_AD_COST_AD + datetime.c_str();

        std::string campaignCmd = "hmget "+GAMES_AD_COST_CAMP + datetime.c_str();
//        campaignCmd.append(GAMES_AD_COST_CAMP);
//        campaignCmd.append(time.c_str());

        std::string adverCmd = "hmget "+GAMES_AD_COST_ADVER + datetime.c_str();

        //昨日花费
        std::string yesterDayCmd = "hmget "+GAMES_AD_COST_ADVER + yestDt;

        std::string creativeCmd = "hmget "+GAMES_AD_COST_CREATIVE + datetime.c_str();
        LOG_DEBUG("time =%s",datetime.c_str());
        //1.获取这个终端查看广告集合的次数

        std::string adNumCmd = "hgetall "+MEM_FREQUENCY+uid;
        std::string dealCmd = "hmget "+MEM_KEY_DEAL_ID;
        std::string deal_campaign = "hmget "+MEM_KEY_DEAL_CAMPAIGN_ID;

        std::vector<std::string>::iterator it;
        std::map<std::string,std::string>::iterator map_it;
        for(int i=0;i<sess->fbreq.ad_size();i++) {
			//2.获取所有的广告存快照
        	if(fbreq.ad(i).adgroup_id() !=0 ) {
        		sess->num = 0;
				sess->num = sess->fbreq.ad(i).adgroup_id();
				it = std::find(sess->adVt.begin(),sess->adVt.end(),util::Func::to_str(sess->num));
				if(it == sess->adVt.end()) {
					adgroupCmd.append(" ");
					adgroupCmd.append(util::Func::to_str(sess->num));
					sess->adVt.push_back(util::Func::to_str(sess->num));
				}
				ab.append(sess->fbreq.ad(i).adgroup_id()+"");
        	}
			//3.获取所有的推广计划内存快照
        	if(sess->fbreq.ad(i).campaign_id()!=0) {
				sess->num = 0;
				sess->num = sess->fbreq.ad(i).campaign_id();
				it = std::find(sess->memCampaignVt.begin(),sess->memCampaignVt.end(),util::Func::to_str(sess->num));
				if(it == sess->memCampaignVt.end()) {
					campaignCmd.append(" ");
					campaignCmd.append(util::Func::to_str(sess->num));
					sess->memCampaignVt.push_back(util::Func::to_str(sess->num));
				}
				ab.append(sess->fbreq.ad(i).campaign_id()+"");
        	}
			//4.获取所有的广告主内存快照

        	if(sess->fbreq.ad(i).inner_advertiser_id()!=0) {
				sess->num = 0;
				sess->num = sess->fbreq.ad(i).inner_advertiser_id();
				it = std::find(sess->advertisterVt.begin(),sess->advertisterVt.end(),util::Func::to_str(sess->num));
				if(it == sess->advertisterVt.end()) {
					adverCmd.append(" ");
					adverCmd.append(util::Func::to_str(sess->num));
					sess->advertisterVt.push_back(util::Func::to_str(sess->num));
				}

				map_it = yesterday_Costmap_.find(yestDt);
				if(map_it == yesterday_Costmap_.end()) {
					yesterday_Costmap_.clear();
					yesterday_Costmap_[yestDt] = "1";

					sess->yesterday_CostVt.push_back(util::Func::to_str(sess->num));
					yesterDayCmd.append(" ");
					yesterDayCmd.append(util::Func::to_str(sess->num));
					yesterday_Costmap_[util::Func::to_str(sess->num)] = "0";

				} else {
					map_it = yesterday_Costmap_.find(util::Func::to_str(sess->num));
					if(map_it == yesterday_Costmap_.end()) {
						sess->yesterday_CostVt.push_back(util::Func::to_str(sess->num));
						yesterDayCmd.append(" ");
						yesterDayCmd.append(util::Func::to_str(sess->num));
						yesterday_Costmap_[util::Func::to_str(sess->num)] = "0";
					}
				}
				ab.append(sess->fbreq.ad(i).inner_advertiser_id()+"");
        	}
			//5.创意ID

        	if(sess->fbreq.ad(i).creative_id()!=0) {
				sess->num = 0;
				sess->num = sess->fbreq.ad(i).creative_id();
				it = std::find(sess->creativeVt.begin(),sess->creativeVt.end(),util::Func::to_str(sess->num));
				if(it == sess->creativeVt.end()) {
					creativeCmd.append(" ");
					creativeCmd.append(util::Func::to_str(sess->num));
					sess->creativeVt.push_back(util::Func::to_str(sess->num));
				}
				ab.append(sess->fbreq.ad(i).creative_id()+"");
        	}

			ab.append(sess->fbreq.ad(i).inner_advertiser_id()+"");
			if(sess->fbreq.ad(i).has_pdb_data()) {
				//交易ID
				if((!fbreq.ad(i).pdb_data().deal_id().empty())
						&& (fbreq.ad(i).pdb_data().deal_id().size()>0)) {

					it = std::find(sess->dealVt.begin(),sess->dealVt.end(),sess->fbreq.ad(i).pdb_data().deal_id().c_str());
					if(it == sess->dealVt.end()) {
						dealCmd.append(" ");
						dealCmd.append(fbreq.ad(i).pdb_data().deal_id().c_str());
						sess->dealVt.push_back(sess->fbreq.ad(i).pdb_data().deal_id().c_str());
					}
					//交易推广计划

					std::string deal_campain_str;

					deal_campain_str.append(sess->fbreq.ad(i).pdb_data().deal_id().c_str());
					deal_campain_str.append(MEM_STATISTICS_COM_DEF.c_str());
//					std::stringstream stream;
//					std::string str;
//					stream << sess->fbreq.ad(i).campaign_id();
//					stream >> str;
					sess->num = sess->fbreq.ad(i).campaign_id();
					deal_campain_str.append(util::Func::to_str(sess->num));


					LOG_DEBUG("deal_campain_str[%s]",deal_campain_str.c_str());
//					deal_campain_str
					it = std::find(sess->deal_campaignVt.begin(),sess->deal_campaignVt.end(),deal_campain_str.c_str());
					if(it == sess->deal_campaignVt.end()) {
						deal_campaign.append(" ");
						deal_campaign.append(deal_campain_str.c_str());
						sess->deal_campaignVt.push_back(deal_campain_str.c_str());
					}
					ab.append(sess->fbreq.ad(i).pdb_data().deal_id().c_str());
				}
			}

			ab.append(" |  ");
			LOG_DEBUG("ab=[%s]",ab.c_str());
        }
        LOG_DEBUG("pid[%d]fbreq[%s]", getpid(), fbreq.DebugString().c_str());
        LOG_DEBUG("campaignCmd[%s]",campaignCmd.c_str());

        if(sess->memCampaignVt.size()>0) {
        	rt=send_get_campaign(sess,campaignCmd.c_str());
        }

        if(rt != 0)
        {
            LOG_ERROR("send_get_campaign error[%d]", rt);
            LOG_DEBUG("redis error noResult tranc_id[%s]",fbreq.trace_id().c_str());
            fbrsp.set_error_code(common::ERROR_NO_RESULT);
            reply_client(sess);
            rt=-1;
            break;
        }

        LOG_DEBUG("adNumCmd[%s]", adNumCmd.c_str());
        rt=send_get_adNum(sess,adNumCmd.c_str());

        if(rt != 0)
		{
			LOG_ERROR("send_get_adNum error[%d]", rt);
			LOG_DEBUG("redis error noResult tranc_id[%s]",fbreq.trace_id().c_str());
			fbrsp.set_error_code(common::ERROR_NO_RESULT);
			reply_client(sess);
			rt=-1;
			break;
		}

        if(sess->advertisterVt.size()>0) {
			LOG_DEBUG("adverCmd[%s]", adverCmd.c_str());
			rt=send_get_advertister(sess,adverCmd.c_str());
        }

		if(rt != 0)
		{
			LOG_ERROR("send_get_advertister error[%d]", rt);
			LOG_DEBUG("redis error noResult tranc_id[%s]",fbreq.trace_id().c_str());
			fbrsp.set_error_code(common::ERROR_NO_RESULT);
			reply_client(sess);
			rt=-1;
			break;
		}

		if(sess->adVt.size()>0) {
			LOG_DEBUG("adgroupCmd[%s]",adgroupCmd.c_str());
			rt=send_get_adgroup(sess,adgroupCmd.c_str());
		}

		if(rt != 0)
		{
			LOG_ERROR("send_get_adgroup error[%d]", rt);
			LOG_DEBUG("redis error noResult tranc_id[%s]",fbreq.trace_id().c_str());
			fbrsp.set_error_code(common::ERROR_NO_RESULT);
			reply_client(sess);
			rt=-1;
			break;
		}

		if(sess->creativeVt.size()>0) {
			LOG_DEBUG("creativeCmd[%s]",creativeCmd.c_str());
			rt=send_get_creative(sess,creativeCmd.c_str());
		}

		if(rt != 0)
		{
			LOG_ERROR("send_get_creative error[%d]", rt);
			LOG_DEBUG("redis error noResult tranc_id[%s]",fbreq.trace_id().c_str());
			fbrsp.set_error_code(common::ERROR_NO_RESULT);
			reply_client(sess);
			rt=-1;
			break;
		}

		if(sess->dealVt.size() > 0 ) {
			LOG_DEBUG("dealCmd[%s]",dealCmd.c_str());
			rt=send_get_deal(sess,dealCmd.c_str());
		}

		if(rt != 0)
		{
			LOG_ERROR("send_get_deal error[%d]", rt);
			LOG_DEBUG("redis error noResult tranc_id[%s]",fbreq.trace_id().c_str());
			fbrsp.set_error_code(common::ERROR_NO_RESULT);
			reply_client(sess);
			rt=-1;
			break;
		}

		if(sess->deal_campaignVt.size() > 0 ) {
			LOG_DEBUG("deal_campaign[%s]",deal_campaign.c_str());
			rt=send_get_deal_campaign(sess,deal_campaign.c_str());
		}

		if(rt != 0)
		{
			LOG_ERROR("send_get_deal_campaign error[%d]", rt);
			LOG_DEBUG("redis error noResult tranc_id[%s]",fbreq.trace_id().c_str());
			fbrsp.set_error_code(common::ERROR_NO_RESULT);
			reply_client(sess);
			rt=-1;
			break;
		}

		if(sess->yesterday_CostVt.size()>0) {
			LOG_DEBUG("yesterday_CostVt[%s]",yesterDayCmd.c_str());
			rt=send_get_yesterdayCost(sess,yesterDayCmd.c_str());
		}

		if(rt != 0)
		{
			LOG_ERROR("send_get_yesterdayCost error[%d]", rt);
			LOG_DEBUG("redis error noResult tranc_id[%s]",fbreq.trace_id().c_str());
			fbrsp.set_error_code(common::ERROR_NO_RESULT);
			reply_client(sess);
			rt=-1;
			break;
		}
    }while(0);
    if(rt != 0)
    {
        if(sess != NULL)
        {
            free_sess(sess);
        }
    }
    return rt;
}

int FbServer::reply_client(SessData * sess)
{
    int rt=0;
    do{
        FeedbackRequest & fbreq=sess->fbreq;
        FeedbackResponse & fbrsp=sess->fbrsp;
        if(fbreq.has_session_id())
        {
                fbrsp.set_session_id(fbreq.session_id());
        }
        if(fbrsp.has_trace_id())
        {
                fbrsp.set_trace_id(fbreq.trace_id());
        }
        for(int i=0;i<sess->fbreq.ad_size();i++) {
        	common::FeedbackInfo* fb;
        	fb=sess->fbrsp.add_feedbackinfo();
        	if(sess->fbreq.ad(i).has_pdb_data()) {
				std::string dealId=sess->fbreq.ad(i).pdb_data().deal_id().c_str();
				if((!dealId.empty())&&(dealId.size()>0)) {
					common::FeedbackInfo_PdbFeedback pdb = sess->dealMap[dealId];
					dealId.append(":");
					sess->num = sess->fbreq.ad(i).campaign_id();
					dealId.append(util::Func::to_str(sess->num));

					LOG_WARN("deal:campId[%s]\n", dealId.c_str());
					common::FeedbackInfo_PdbFeedback camp_pdb = sess->deal_campaignMap[dealId.c_str()];
					LOG_WARN("camp_pdb.deal_campaign_day_exp()[%d]\n",camp_pdb.deal_campaign_day_exp());
					pdb.set_deal_campaign_day_exp(camp_pdb.deal_campaign_day_exp());
					fb->mutable_pdb_feedback()->CopyFrom(pdb);
				}
			}
        	fb->set_adgroup_id(sess->fbreq.ad(i).adgroup_id());
        	fb->set_campaign_id(sess->fbreq.ad(i).campaign_id());
        	fb->set_advertiser_id(sess->fbreq.ad(i).inner_advertiser_id());
        	fb->set_aid(sess->fbreq.aid());
        	fb->set_acookie(sess->fbreq.acookie());
        	fb->set_dev_id(sess->fbreq.dev_id());


			sess->num = sess->fbreq.ad(i).inner_advertiser_id();
			std::string memAdvertister = sess->advertisterMap[util::Func::to_str(sess->num)];
			if((!memAdvertister.empty()) &&(memAdvertister.size()>0) ){
				fb->set_advertiser_day_cost(atol(memAdvertister.c_str())/1000L);
			}else{
				fb->set_advertiser_day_cost(0);
			}

			sess->num = sess->fbreq.ad(i).campaign_id();
			std::string memCampaign =sess->memCampaignMap[util::Func::to_str(sess->num)];
			LOG_DEBUG("memCampaign[%s]=",memCampaign.c_str());
			if(!memCampaign.empty() && memCampaign.length()>0) {
				LOG_DEBUG("memCampaign[%s]=",memCampaign.c_str());
				fb->set_campaign_day_cost(atol(memCampaign.c_str())/1000L);
			} else {
				fb->set_campaign_day_cost(0);
			}

			common::FeedbackInfo::CreativeCost* creativeCost = fb->add_creative_cost();
			//创意
			sess->num = sess->fbreq.ad(i).creative_id();
			std::string creative_cost =sess->creativeMap[util::Func::to_str(sess->num)];
			LOG_DEBUG("creative_cost[%s]=",creative_cost.c_str());
			if(!creative_cost.empty() && creative_cost.length()>0) {
				creativeCost->set_cid(sess->num);
				creativeCost->set_creative_day_cost(atol(creative_cost.c_str())/1000L);
				LOG_DEBUG("creative_cost[%s]=",creative_cost.c_str());
			} else {
				creativeCost->set_creative_day_cost(0);
			}

			sess->num = sess->fbreq.ad(i).adgroup_id();
			std::string memAd =sess->adMap[util::Func::to_str(sess->num)];
			if(!memAd.empty() && memAd.length()>0) {
				fb->set_adgroup_day_cost(atol(memAd.c_str())/1000L);
			} else {
				fb->set_adgroup_day_cost(0);
			}

			sess->num = fbreq.ad(i).inner_advertiser_id();
			sess->str.clear();
			sess->str = MEM_KEY_ADVER;
			sess->str.append(util::Func::to_str(sess->num));
			std::string mem_adver_count =sess->countMap[sess->str];
			if(!mem_adver_count.empty() && mem_adver_count.length()>0) {
				fb->set_advertiser_user_day_freq(atol(mem_adver_count.c_str()));
			} else {
				fb->set_advertiser_user_day_freq(0);
			}



			sess->str.clear();
			sess->str = MEM_KEY_CAMP;
			sess->num = fbreq.ad(i).campaign_id();
			sess->str.append(util::Func::to_str(sess->num));
			std::string mem_camp_count =sess->countMap[sess->str];


			if(!mem_camp_count.empty() && mem_camp_count.length()>0) {
				fb->set_campaign_user_day_freq(atol(mem_camp_count.c_str()));
			} else {
				fb->set_campaign_user_day_freq(0);
			}

			if(!sess->fbreq.ad(i).advertiser_balance_day().empty()) {
				sess->num = fbreq.ad(i).inner_advertiser_id();
//				sess->num = util::Func::to_int(yesterday_Costmap_[util::Func::to_str(sess->num)]);
				fb->set_advertiser_last_day_cost(atol(yesterday_Costmap_[util::Func::to_str(sess->num)].c_str())/1000L);
			}

			sess->str.clear();
			sess->str = MEM_KEY_AD;
			sess->num = sess->fbreq.ad(i).adgroup_id();
			sess->str.append(util::Func::to_str(sess->num));
			std::string mem_ad_count =sess->countMap[sess->str];
			if(!mem_ad_count.empty()) {
				fb->set_adgroup_user_day_freq(util::Func::to_int(mem_ad_count.c_str()));
			}
			else{
				fb->set_adgroup_user_day_freq(0);
			}

			//创意次数
			sess->str.clear();
			sess->str = MEM_KEY_CREATIVE;
			sess->num = sess->fbreq.ad(i).creative_id();
			sess->str.append(util::Func::to_str(sess->num));
			std::string creative_ad_count =sess->countMap[sess->str];
			if(fbreq.ad(i).has_creative_id()) {
				creativeCost->set_cid(sess->num);
			}
			if(!creative_ad_count.empty()) {
				creativeCost->set_cid(sess->num);
				creativeCost->set_creative_day_freq(util::Func::to_int(creative_ad_count.c_str()));
			}
			else{
				creativeCost->set_creative_day_freq(0);
			}

        }
        sess->fbrsp.set_error_code(common::ERROR_NONE);
        std::string sendbuf;
        if(!fbrsp.SerializeToString(&sendbuf))
        {
            MON_ADD(ATTR_PACK_ERROR, 1);
            LOG_ERROR("SerializeToString error trance_id[%s]",fbreq.trace_id().c_str());
            rt=-1;
            break;
        }
        LOG_DEBUG("dnrsp[%s]\n", fbrsp.DebugString().c_str());

        if(!Config::get_mutable_instance().is_dumb())
        {
            send_pkg(sendbuf.c_str(), sendbuf.length(), sess->addr);
        }
        MON_ADD(ATTR_FB_RSP, 1);
    }while(0);
    free_sess(sess);
    return rt;
}

int FbServer::send_get_adgroup(SessData * sess,std::string cmd)
{
    int rt=0;
    do{
        RedisAccess * ra=RedisPool::get_mutable_instance().get_redis_instance(REDIS_ADKEY);
        if(ra == NULL)
        {
            MON_ADD(ATTR_GET_REDIS_ERR, 1);
            LOG_ERROR("get_redis_instance error");
            rt=-1;
            break;
        }
        LOG_DEBUG("cmd[%s] tranceid[%s]", cmd.c_str(),sess->fbreq.trace_id().c_str());
        rt=ra->send_cmd(sess, cmd.c_str());
        if(rt != 0)
        {
                LOG_DEBUG("cmds[%s] tranceid[%s]", cmd.c_str(),sess->fbreq.trace_id().c_str());
            MON_ADD(ATTR_SEND_CMD_ERROR, 1);
            break;
        }
        sess->status ^= ADKEY;
        LOG_DEBUG("send_get_adgroup   sess->status=[%d]",sess->status);
    }while(0);
    return rt;
}

int FbServer::send_get_campaign(SessData * sess,std::string cmd)
{
    int rt=0;
    do{
        RedisAccess * ra=RedisPool::get_mutable_instance().get_redis_instance(REDIS_MEMCAMPAIGN);
        if(ra == NULL)
        {
            MON_ADD(ATTR_GET_REDIS_ERR, 1);
            LOG_ERROR("get_redis_instance error");
            rt=-1;
            break;
        }
        LOG_DEBUG("cmd[%s] tranceid[%s]", cmd.c_str(),sess->fbreq.trace_id().c_str());
        rt=ra->send_cmd(sess, cmd.c_str());
        LOG_DEBUG("rt=[%d]",rt);
        if(rt != 0)
        {
                LOG_DEBUG("cmds[%s] tranceid[%s]", cmd.c_str(),sess->fbreq.trace_id().c_str());
            MON_ADD(ATTR_SEND_CMD_ERROR, 1);
            break;
        }
        sess->status^=MEMCAMPAIGN;
        LOG_DEBUG("send_get_campaign   sess->status=[%d]",sess->status);
    }while(0);
    return rt;
}

int FbServer::send_get_advertister(SessData * sess,std::string cmd)
{
    int rt=0;
    do{
        RedisAccess * ra=RedisPool::get_mutable_instance().get_redis_instance(REDIS_ADVERTISTER);
        if(ra == NULL)
        {
            MON_ADD(ATTR_GET_REDIS_ERR, 1);
            LOG_ERROR("get_redis_instance error");
            rt=-1;
            break;
        }
        LOG_DEBUG("cmd[%s] tranceid[%s]", cmd.c_str(),sess->fbreq.trace_id().c_str());
        rt=ra->send_cmd(sess, cmd.c_str());
        if(rt != 0)
        {
                LOG_DEBUG("cmds[%s] tranceid[%s]", cmd.c_str(),sess->fbreq.trace_id().c_str());
            MON_ADD(ATTR_SEND_CMD_ERROR, 1);
            break;
        }
        sess->status ^= ADVERTISTER;
        LOG_DEBUG("send_get_advertister   sess->status=[%d]",sess->status);
    }while(0);
    return rt;
}

int FbServer::send_get_creative(SessData * sess,std::string cmd)
{
    int rt=0;
    do{
        RedisAccess * ra=RedisPool::get_mutable_instance().get_redis_instance(REDIS_CREATIVE);
        if(ra == NULL)
        {
            MON_ADD(ATTR_GET_REDIS_ERR, 1);
            LOG_ERROR("get_redis_instance error");
            rt=-1;
            break;
        }
        LOG_DEBUG("cmd[%s] tranceid[%s]", cmd.c_str(),sess->fbreq.trace_id().c_str());
        rt=ra->send_cmd(sess, cmd.c_str());
        if(rt != 0)
        {
            LOG_DEBUG("cmds[%s] tranceid[%s]", cmd.c_str(),sess->fbreq.trace_id().c_str());
            MON_ADD(ATTR_SEND_CMD_ERROR, 1);
            break;
        }
        sess->status ^= CREATIVE;
        LOG_DEBUG("send_get_creative   sess->status=[%d]",sess->status);
    }while(0);
    return rt;
}

int FbServer::send_get_adNum(SessData * sess,std::string cmd)
{
    int rt=0;
    do{
        RedisAccess * ra=RedisPool::get_mutable_instance().get_redis_instance(REDIS_ADNUM);
        if(ra == NULL)
        {
            MON_ADD(ATTR_GET_REDIS_ERR, 1);
            LOG_ERROR("get_redis_instance error");
            rt=-1;
            break;
        }
        LOG_DEBUG("cmd[%s] tranceid[%s]", cmd.c_str(),sess->fbreq.trace_id().c_str());
        rt=ra->send_cmd(sess, cmd.c_str());
        if(rt != 0)
        {
                LOG_DEBUG("cmds[%s] tranceid[%s]", cmd.c_str(),sess->fbreq.trace_id().c_str());
            MON_ADD(ATTR_SEND_CMD_ERROR, 1);
            break;
        }
        sess->status ^= ADNUM;
        LOG_DEBUG("send_get_adNum   sess->status=[%d]",sess->status);
    }while(0);
    return rt;
}

int FbServer::send_get_deal(SessData * sess,std::string cmd)
{
    int rt=0;
    do{
        RedisAccess * ra=RedisPool::get_mutable_instance().get_redis_instance(REDIS_DEAL_ID);
        if(ra == NULL)
        {
            MON_ADD(ATTR_GET_REDIS_ERR, 1);
            LOG_ERROR("get_redis_instance error");
            rt=-1;
            break;
        }
        LOG_DEBUG("cmd[%s] tranceid[%s]", cmd.c_str(),sess->fbreq.trace_id().c_str());
        rt=ra->send_cmd(sess, cmd.c_str());
        if(rt != 0)
        {
                LOG_DEBUG("cmds[%s] tranceid[%s]", cmd.c_str(),sess->fbreq.trace_id().c_str());
            MON_ADD(ATTR_SEND_CMD_ERROR, 1);
            break;
        }
        sess->status ^= DEAL_ID;
        LOG_DEBUG("send_get_deal   sess->status=[%d]",sess->status);
    }while(0);
    return rt;
}

int FbServer::send_get_deal_campaign(SessData * sess,std::string cmd)
{
    int rt=0;
    do{
        RedisAccess * ra=RedisPool::get_mutable_instance().get_redis_instance(REDIS_DEAL_CAMPAIGN);
        if(ra == NULL)
        {
            MON_ADD(ATTR_GET_REDIS_ERR, 1);
            LOG_ERROR("get_redis_instance error");
            rt=-1;
            break;
        }
        LOG_DEBUG("cmd[%s] tranceid[%s]", cmd.c_str(),sess->fbreq.trace_id().c_str());
        rt=ra->send_cmd(sess, cmd.c_str());
        if(rt != 0)
        {
                LOG_DEBUG("cmds[%s] tranceid[%s]", cmd.c_str(),sess->fbreq.trace_id().c_str());
            MON_ADD(ATTR_SEND_CMD_ERROR, 1);
            break;
        }
        sess->status ^= DEAL_CAMPAIGN;
        LOG_DEBUG("send_get_deal_campaign   sess->status=[%d]",sess->status);
    }while(0);
    return rt;
}

int FbServer::send_get_yesterdayCost(SessData * sess,std::string cmd)
{
    int rt=0;
    do{
        RedisAccess * ra=RedisPool::get_mutable_instance().get_redis_instance(YESTERDAY_COST);
        if(ra == NULL)
        {
            MON_ADD(ATTR_GET_REDIS_ERR, 1);
            LOG_ERROR("get_redis_instance error");
            rt=-1;
            break;
        }
        LOG_DEBUG("cmd[%s] tranceid[%s]", cmd.c_str(),sess->fbreq.trace_id().c_str());
        rt=ra->send_cmd(sess, cmd.c_str());
        if(rt != 0)
        {
			LOG_DEBUG("cmds[%s] tranceid[%s]", cmd.c_str(),sess->fbreq.trace_id().c_str());
            MON_ADD(ATTR_SEND_CMD_ERROR, 1);
            break;
        }
        sess->status ^= YESTERDAYCOST;
        LOG_DEBUG("send_get_yesterdayCost   sess->status=[%d]",sess->status);
    }while(0);
    return rt;
}

int FbServer::on_get_adgroup(void * data, void * reply, int err_code, const char * err_str)
{
    int rt=0;
    rt = getFromRedis(data,reply,err_code,err_str,ADKEY);
    return rt;
}

int FbServer::on_get_campaign(void * data, void * reply, int err_code, const char * err_str )
{
    int rt=0;
    rt = getFromRedis(data,reply,err_code,err_str,MEMCAMPAIGN);
    return rt;
}

int FbServer::on_get_creative(void * data, void * reply, int err_code, const char * err_str )
{
    int rt = 0;
    rt = getFromRedis(data,reply,err_code,err_str,CREATIVE);
    return rt;
}

int FbServer::on_get_count(void * data, void * reply, int err_code, const char * err_str )
{
    int rt = 0;
    rt = getFromRedis(data,reply,err_code,err_str,ADNUM);
    return rt;
}

int FbServer::on_get_advertister(void * data, void * reply, int err_code, const char * err_str )
{
	int rt;
	rt = getFromRedis(data,reply,err_code,err_str,ADVERTISTER);
    return rt;
}

int FbServer::on_get_deal_campaign(void * data, void * reply, int err_code, const char * err_str )
{
	int rt;
	rt = getFromRedis(data,reply,err_code,err_str,DEAL_CAMPAIGN);
    return rt;
}

int FbServer::on_get_deal(void * data, void * reply, int err_code, const char * err_str )
{
	int rt;
	rt = getFromRedis(data,reply,err_code,err_str,DEAL_ID);
    return rt;
}

int FbServer::on_get_yesterdayCost(void * data, void * reply, int err_code, const char * err_str )
{
	int rt;
	rt = getFromRedis(data,reply,err_code,err_str,YESTERDAYCOST);
    return rt;
}

int FbServer::getFromRedis(void * data, void * reply, int err_code, const char * err_str,int status)
{
    int rt=0;
    SessData * sess=(SessData *)data;
    LOG_DEBUG("sess taracid[%s]", sess->fbreq.trace_id().c_str());
    redisReply * pReply=(::redisReply *)reply;
    do{
        if(!sess_valid(sess))
        {
            LOG_DEBUG("sess invalid[%d]", getpid());
            rt=-1;
            break;
        }
        if(err_code != 0)
        {
            LOG_ERROR("getFromRedis error[%d][%s]trancid[%s]\n", err_code, err_str,sess->fbreq.trace_id().c_str());
        }else if(pReply == NULL )
        {
            LOG_ERROR("pReply == NULL");
        }else if(pReply->type != REDIS_REPLY_ARRAY)
        {
            if(pReply->type == REDIS_REPLY_NIL)
            {
                LOG_DEBUG("ATTR_CREATIVE_NO_KEY trancid[%s]\n",sess->fbreq.trace_id().c_str());
                LOG_ERROR("ATTR_CREATIVE_NO_KEY trancid[%s]\n",sess->fbreq.trace_id().c_str());
            }
            if(pReply->type == REDIS_REPLY_ERROR)
            {
                LOG_DEBUG("getFromRedis->type != STRING[%d][%s] trancid[%s]", pReply->type, pReply->str,sess->fbreq.trace_id().c_str());
                LOG_ERROR("getFromRedis->type != STRING[%d][%s]", pReply->type, pReply->str);
            }
            LOG_DEBUG("getFromRedis->type != STRING[%d]", pReply->type);
        } else
        {
			if (pReply->type == REDIS_REPLY_ARRAY) {
					std::vector<std::string>::iterator memCampaignit;
					memCampaignit = sess->memCampaignVt.begin();

					std::vector<std::string>::iterator advertisterit;
					advertisterit = sess->advertisterVt.begin();

					std::vector<std::string>::iterator adit;
					adit = sess->adVt.begin();

					std::vector<std::string>::iterator creativeit;
					creativeit = sess->creativeVt.begin();

					std::vector<std::string>::iterator dealit;
					dealit = sess->dealVt.begin();

					std::vector<std::string>::iterator deal_campaignit;
					deal_campaignit = sess->deal_campaignVt.begin();

					std::vector<std::string>::iterator yesterday_Costit;
					yesterday_Costit = sess->yesterday_CostVt.begin();

					for (int i = 0; i < pReply->elements; i++) {
						redisReply* childReply = pReply->element[i];
						LOG_DEBUG("getFromRedis trancid[%s]",sess->fbreq.trace_id().c_str());
						LOG_DEBUG("sess->status=[%d]",status);
						LOG_DEBUG("childReply->str=[%s]",childReply->str);
						if(childReply->type == REDIS_REPLY_STRING) {
							do{
							switch(status) {
								case MEMCAMPAIGN:{
									sess->memCampaignMap[*memCampaignit] = childReply->str;
									break;
								}
								case ADVERTISTER:
									sess->advertisterMap[*advertisterit] = childReply->str;
									break;
								case ADKEY:
									sess->adMap[*adit++] = childReply->str;
									break;
								case ADNUM:{
									redisReply* valueReply = pReply->element[++i];
									LOG_DEBUG("childReply=[%s]",childReply->str);
									LOG_DEBUG("valueReply=[%s]",valueReply->str);
									sess->countMap[childReply->str] = valueReply->str;
									break;
								}
								case CREATIVE:
									sess->creativeMap[*creativeit] = childReply->str;
									break;
								case DEAL_ID:{
									common::FeedbackInfo_PdbFeedback pdb;
									sess->str.clear();
									sess->str = *dealit;
									pdb.set_deal_id(sess->str);
									pdb.set_deal_day_exp(util::Func::to_int(childReply->str));
									sess->dealMap[sess->str] = pdb;
									break;
								}
								case DEAL_CAMPAIGN:{
									common::FeedbackInfo_PdbFeedback pdb;
									sess->str.clear();
									sess->str = *deal_campaignit;
//									pdb.set_deal_id(sess->str);
									LOG_WARN("deal:campId[%s]\n", sess->str.c_str());
									LOG_WARN("deal:campId_value[%s]\n", childReply->str);
									pdb.set_deal_campaign_day_exp(util::Func::to_int(childReply->str));
									sess->deal_campaignMap[sess->str.c_str()] = pdb;
									break;
								}
								case YESTERDAYCOST:
									yesterday_Costmap_[*yesterday_Costit] = childReply->str;
									break;
								default:
									LOG_WARN("unkowned status[%d]\n", sess->status);
									break;
								}
							}while(0);
						}
						if(MEMCAMPAIGN == status) {
							memCampaignit++;
						}
						if(ADVERTISTER == status) {
							advertisterit++;
						}
						if(ADKEY == status) {
							adit++;
						}
						if(CREATIVE == status) {
							creativeit++;
						}
						if((DEAL_ID == status)) {
							dealit++;
						}
						if(DEAL_CAMPAIGN == status) {
							deal_campaignit++;
						}
						if(YESTERDAYCOST == status) {
							yesterday_Costit++;
						}
					}
				}
			}
       }while(0);
    sess->status ^= status;
    LOG_DEBUG("redis   sess->status=[%d]",sess->status);
    rt=proc_sess(sess);
    return rt;
}

int FbServer::proc_sess(SessData * sess)
{
    int rt=0;
    do{
    	LOG_DEBUG("proc_sess sess->status=[%d]",sess->status);
        if( sess->status != 0 )
        {
            break;
        }
        FeedbackRequest fbreq = sess->fbreq;
        FeedbackResponse fbrsp = sess->fbrsp;
        fbrsp.set_error_code(common::ERROR_NONE);
        LOG_DEBUG("proc_sess");
        reply_client(sess);
    }while(0);
    return rt;
}

int FbServer::s_on_get_adgroup(void * data, void * reply, int err_code, const char * err_str )
{
    return FbServer::get_mutable_instance().on_get_adgroup(data, reply, err_code, err_str);
}

int FbServer::s_on_get_campaign(void * data, void * reply, int err_code, const char * err_str )
{
    return FbServer::get_mutable_instance().on_get_campaign(data, reply, err_code, err_str);
}

int FbServer::s_on_get_count(void * data, void * reply, int err_code, const char * err_str )
{
    return FbServer::get_mutable_instance().on_get_count(data, reply, err_code, err_str);
}

int FbServer::s_on_get_advertister(void * data, void * reply, int err_code, const char * err_str )
{
    return FbServer::get_mutable_instance().on_get_advertister(data, reply, err_code, err_str);
}

int FbServer::s_on_get_creative(void * data, void * reply, int err_code, const char * err_str )
{
    return FbServer::get_mutable_instance().on_get_creative(data, reply, err_code, err_str);
}

int FbServer::s_on_get_deal(void * data, void * reply, int err_code, const char * err_str )
{
    return FbServer::get_mutable_instance().on_get_deal(data, reply, err_code, err_str);
}

int FbServer::s_on_get_deal_campaign(void * data, void * reply, int err_code, const char * err_str )
{
    return FbServer::get_mutable_instance().on_get_deal_campaign(data, reply, err_code, err_str);
}

int FbServer::s_on_get_yesterdayCost(void * data, void * reply, int err_code, const char * err_str )
{
    return FbServer::get_mutable_instance().on_get_yesterdayCost(data, reply, err_code, err_str);
}
}
}
