/**
 **/

#include "process_stat.h"

#include <iostream>
#include <string>

#include "config.h"
#include "hiredis.h"
#include "util/func.h"
#include "util/log.h"
#include "util/util_str.h"
#include "monitor_api.h"
#include "log_server_attr.h"

namespace poseidon
{
namespace log
{

/**
 * @brief           初始化
 **/
int ProcessStat::init()
{
    int rt=0;
    do{
        redis_host_=Config::get_mutable_instance().get_redis_host();
        redis_port_=Config::get_mutable_instance().get_redis_port();
        rt=redis_connect();
        if(rt != 0)
        {
            LOG_ERROR("redis_connect error");

            break;
        }
    }while(0);
    return rt;
}

/**
 * @brief               redis重新连接
 **/
int ProcessStat::redis_connect()
{
    int rt=0;
    do{
        redis_context_=redisConnect(redis_host_.c_str(), redis_port_);
        if(redis_context_->err)
        {
            LOG_ERROR("redisConnect error[%s]\n", redis_context_->errstr);
            MON_ADD(ATTR_REDIS_WRITE_ERR, 1);          
            redisFree(redis_context_);  
            redis_context_=NULL;
            rt=-1;
        }
    }while(0);
    return rt;
}


int ProcessStat::stat_source_ad_pv(const std::string & source, int req_ad_num)
{
    int rt=0;
    do{
        time_t now=time(NULL);
        std::map<time_t, int>& source_map=sources_ad_st_[source];

//        std::pair<const std::string, time_t> pair=std::make_pair(source, now);
        if(source_map.count(now) > 0)
        {
            source_map[now]+=req_ad_num;
        }else
        {
            std::map<time_t, int>::iterator it;
            for(it=source_map.begin(); it != source_map.end(); it++)
            {
                int count=it->second;
                
                std::stringstream ss;
          
                //INCRBY KEY_NAME FIELD_NAME INCR_BY_NUMBER
                //keyname 
                ss<<"hincrby rts_ad_req_pv_ex:";
                std::string date;
                util::Func::get_time_str(&date, "%Y%m%d");
                ss<<date<<"0000:1-D:source";
         
                ss<<" ";
                //field_name;
                ss<<"req_pv:"<<source;

                ss<<" ";
                ss<<count;
          
                std::string cmd=ss.str();
                LOG_DEBUG("redis cmd[%s]", cmd.c_str());
                
          
                //step 4:执行redis命令
                if(redis_context_==NULL)
                {
                    rt=redis_connect();
                    if(rt != 0)
                    {
                        break;
                    }
                }

                MON_ADD(ATTR_REDIS_WRITE, 1);          
                redisReply * r =(redisReply *)redisCommand(redis_context_, cmd.c_str());
                if(r==NULL)
                {
                    LOG_ERROR("redis execute error, cmd[%s]", cmd.c_str());
                    MON_ADD(ATTR_REDIS_WRITE_ERR, 1);          
                    redisFree(redis_context_);
                    redis_context_=NULL;
                    redis_connect();
                    break;
                }else
                {
                    freeReplyObject(r);
                }
                
            }
            source_map.clear();
            source_map[now]=req_ad_num;
        }
    }while(0);
    return rt;
}

int ProcessStat::stat_source_pv(const std::string & source)
{
    int rt=0;
    do{
        time_t now=time(NULL);
        std::map<time_t, int>& source_map=sources_st_[source];

//        std::pair<const std::string, time_t> pair=std::make_pair(source, now);
        if(source_map.count(now) > 0)
        {
            source_map[now]++;
        }else
        {
            std::map<time_t, int>::iterator it;
            for(it=source_map.begin(); it != source_map.end(); it++)
            {
                int count=it->second;
                
                std::stringstream ss;
          
                //INCRBY KEY_NAME FIELD_NAME INCR_BY_NUMBER
                //keyname 
                ss<<"hincrby rts_ad_req_pv:";
                std::string date;
                util::Func::get_time_str(&date, "%Y%m%d");
                ss<<date<<"0000:1-D:source";
         
                ss<<" ";
                //field_name;
                ss<<"req_pv:"<<source;

                ss<<" ";
                ss<<count;
          
                std::string cmd=ss.str();
                LOG_DEBUG("redis cmd[%s]", cmd.c_str());
                
          
                //step 4:执行redis命令
                if(redis_context_==NULL)
                {
                    rt=redis_connect();
                    if(rt != 0)
                    {
                        break;
                    }
                }
          
                MON_ADD(ATTR_REDIS_WRITE, 1);          
                redisReply * r =(redisReply *)redisCommand(redis_context_, cmd.c_str());
                if(r==NULL)
                {
                    LOG_ERROR("redis execute error, cmd[%s]", cmd.c_str());
                    MON_ADD(ATTR_REDIS_WRITE_ERR, 1);          
                    redisFree(redis_context_);
                    redis_context_=NULL;
                    redis_connect();
                    break;
                }else
                {
                    freeReplyObject(r);
                }
            }
            source_map.clear();
            source_map[now]=1;
        }
    }while(0);
    return rt;
}

int ProcessStat::stat_source_exp_id_pv(const std::string & source,const std::string & exp_id,int bid_type)
{
    int rt=0;
	std::stringstream ss;
	std::stringstream ss_hour;
	//INCRBY KEY_NAME FIELD_NAME INCR_BY_NUMBER
	//keyname
	ss<<"hincrby rts_ad_exp_req_pv:";
	ss_hour<<"hincrby rts_ad_exp_req_pv:";
	std::string date;
	util::Func::get_time_str(&date, "%Y%m%d");
	std::string date_hour;
	util::Func::get_time_str(&date_hour, "%Y%m%d%H");
	ss<<date<<"0000:1-D:source`exp_id";
	ss_hour<<date_hour<<"00:1-D:source`exp_id";
	ss<<" ";
	ss_hour<<" ";
	//field_name;
	ss<<"bid_pv:"<<source;
	ss_hour<<"bid_pv:"<<source;
	ss<<"`"<<exp_id;
	ss_hour<<"`"<<exp_id;
	ss<<" ";
	ss_hour<<" ";
	ss<<1;
	ss_hour<<1;
	std::string cmd_bid=ss.str();
	std::string cmd_bid_hour=ss_hour.str();
	LOG_DEBUG("redis cmd_bid[%s]", cmd_bid.c_str());
	LOG_DEBUG("redis cmd_bid_hour[%s]", cmd_bid_hour.c_str());

	if(redis_context_==NULL)
	{
		rt=redis_connect();
		if(rt != 0)
		{
			return rt;
		}
	}

	redisReply * r =(redisReply *)redisCommand(redis_context_, cmd_bid.c_str());
	redisReply * r_hour =(redisReply *)redisCommand(redis_context_, cmd_bid_hour.c_str());
	if(r==NULL) {
		LOG_ERROR("redis execute error, cmd[%s]", cmd_bid.c_str());
		redisFree(redis_context_);
		redis_context_=NULL;
		redis_connect();
		return rt;
	} else {
		freeReplyObject(r);
	}
	if(r_hour==NULL) {
		LOG_ERROR("redis execute error, cmd[%s]",cmd_bid_hour.c_str());
		redisFree(redis_context_);
		redis_context_=NULL;
		redis_connect();
		return rt;
	} else {
		freeReplyObject(r_hour);
	}
    return rt;
}

int ProcessStat::stat_source_exp_id_all_pv(const std::string & source,int bid_type)
{
    int rt=0;
    do{
        time_t now=time(NULL);
        std::map<time_t, int>& source_map=sources_exp_st_all[source];
//        std::pair<const std::string, time_t> pair=std::make_pair(source, now);
        if(source_map.count(now) > 0)
        {
            source_map[now]++;
        }else
        {
            std::map<time_t, int>::iterator it;
            for(it=source_map.begin(); it != source_map.end(); it++)
            {
                int count=it->second;
                std::stringstream ss;
                std::stringstream ss_hour;
                //INCRBY KEY_NAME FIELD_NAME INCR_BY_NUMBER
                //keyname
                ss<<"hincrby rts_ad_exp_req_pv:";
                ss_hour<<"hincrby rts_ad_exp_req_pv:";
                std::string date;
                util::Func::get_time_str(&date, "%Y%m%d");
                std::string date_hour;
				util::Func::get_time_str(&date_hour, "%Y%m%d%H");
                ss<<date<<"0000:1-D:source";
                ss_hour<<date_hour<<"00:1-D:source";
                ss<<" ";
                ss_hour<<" ";
                //field_name;
                ss<<"req_pv:"<<source;
                ss_hour<<"req_pv:"<<source;
                ss<<" ";
                ss_hour<<" ";
                ss<<count;
                ss_hour<<count;
                std::string cmd=ss.str();
                std::string cmd_hour=ss_hour.str();
                LOG_DEBUG("redis cmd[%s]", cmd.c_str());
                LOG_DEBUG("redis cmd_hour[%s]", cmd_hour.c_str());
                std::string cmd_bid;
                std::string cmd_bid_hour;
                if(bid_type == 1) {
					//INCRBY KEY_NAME FIELD_NAME INCR_BY_NUMBER
					//keyname
                	std::stringstream ss_bid;
                	std::stringstream ss_bid_hour;
					ss_bid<<"hincrby rts_ad_exp_req_pv:";
					ss_bid_hour<<"hincrby rts_ad_exp_req_pv:";
					std::string date;
					util::Func::get_time_str(&date, "%Y%m%d");
					std::string date_hour;
					util::Func::get_time_str(&date_hour, "%Y%m%d%H");
					ss_bid<<date<<"0000:1-D:source";
					ss_bid_hour<<date_hour<<"00:1-D:source";
					ss_bid<<" ";
					ss_bid_hour<<" ";
					//field_name;
					ss_bid<<"bid_pv:"<<source;
					ss_bid_hour<<"bid_pv:"<<source;
					ss_bid<<" ";
					ss_bid_hour<<" ";
					ss_bid<<count;
					ss_bid_hour<<count;
					cmd_bid=ss_bid.str();
					cmd_bid_hour=ss_bid_hour.str();
					LOG_DEBUG("redis cmd_bid[%s]", cmd_bid.c_str());
					LOG_DEBUG("redis cmd_bid_hour[%s]", cmd_bid_hour.c_str());
                }
                //step 4:执行redis命令
                if(redis_context_==NULL)
                {
                    rt=redis_connect();
                    if(rt != 0)
                    {
                        break;
                    }
                }

                redisReply * r =(redisReply *)redisCommand(redis_context_, cmd.c_str());
                redisReply * r_hour =(redisReply *)redisCommand(redis_context_, cmd_hour.c_str());
                if(bid_type ==1) {
                	redisReply * r_bid =(redisReply *)redisCommand(redis_context_, cmd_bid.c_str());
                	redisReply * r_bid_hour =(redisReply *)redisCommand(redis_context_, cmd_bid_hour.c_str());
					if(r==NULL) {
						LOG_ERROR("redis execute error, cmd[%s]", cmd.c_str());
						MON_ADD(ATTR_REDIS_WRITE_ERR, 1);
						redisFree(redis_context_);
						redis_context_=NULL;
						redis_connect();
						break;
					} else {
						freeReplyObject(r);
					}
					if(r_hour==NULL) {
						LOG_ERROR("redis execute error, cmd[%s]", cmd_hour.c_str());
						MON_ADD(ATTR_REDIS_WRITE_ERR, 1);
						redisFree(redis_context_);
						redis_context_=NULL;
						redis_connect();
						break;
					} else {
						freeReplyObject(r_hour);
					}
					if(r_bid==NULL) {
						LOG_ERROR("redis execute error, cmd[%s]", cmd_bid.c_str());
						MON_ADD(ATTR_REDIS_WRITE_ERR, 1);
						redisFree(redis_context_);
						redis_context_=NULL;
						redis_connect();
						break;
					} else {
						freeReplyObject(r_bid);
					}
					if(r_bid_hour==NULL) {
						LOG_ERROR("redis execute error, cmd[%s]", cmd_bid_hour.c_str());
						MON_ADD(ATTR_REDIS_WRITE_ERR, 1);
						redisFree(redis_context_);
						redis_context_=NULL;
						redis_connect();
						break;
					} else {
						freeReplyObject(r_bid_hour);
					}
                } else {
					if(r==NULL) {
						LOG_ERROR("redis execute error, cmd[%s]", cmd.c_str());
						redisFree(redis_context_);
						redis_context_=NULL;
						redis_connect();
						break;
					} else {
						freeReplyObject(r);
					}
					if(r_hour==NULL) {
						LOG_ERROR("redis execute error, cmd[%s]", cmd_hour.c_str());
						redisFree(redis_context_);
						redis_context_=NULL;
						redis_connect();
						break;
					} else {
						freeReplyObject(r_hour);
					}
                }
            }
            source_map.clear();
            source_map[now]=1;
        }
    }while(0);
    return rt;
}


int ProcessStat::stat_source_exp_id_bid_all_pv(const std::string & source,int bid_type)
{
    int rt=0;
    do{
        time_t now=time(NULL);
        std::map<time_t, int>& source_map=sources_exp_st_bid_all[source];
//        std::pair<const std::string, time_t> pair=std::make_pair(source, now);
        if(source_map.count(now) > 0)
        {
            source_map[now]++;
        }else
        {
            std::map<time_t, int>::iterator it;
            for(it=source_map.begin(); it != source_map.end(); it++)
            {
                int count=it->second;
                std::stringstream ss;
                std::stringstream ss_hour;
                //INCRBY KEY_NAME FIELD_NAME INCR_BY_NUMBER
                //keyname
                ss<<"hincrby rts_ad_exp_req_pv:";
                ss_hour<<"hincrby rts_ad_exp_req_pv:";
                std::string date;
                util::Func::get_time_str(&date, "%Y%m%d");
                std::string date_hour;
				util::Func::get_time_str(&date_hour, "%Y%m%d%H");
                ss<<date<<"0000:1-D:source";
                ss_hour<<date_hour<<"00:1-D:source";
                ss<<" ";
                ss_hour<<" ";
                //field_name;
                ss<<"req_pv:"<<source;
                ss_hour<<"req_pv:"<<source;
                ss<<" ";
                ss_hour<<" ";
                ss<<count;
                ss_hour<<count;
                std::string cmd=ss.str();
                std::string cmd_hour=ss_hour.str();
                LOG_DEBUG("redis cmd[%s]", cmd.c_str());
                LOG_DEBUG("redis cmd_hour[%s]", cmd_hour.c_str());
                std::string cmd_bid;
                std::string cmd_bid_hour;
                if(bid_type == 1) {
					//INCRBY KEY_NAME FIELD_NAME INCR_BY_NUMBER
					//keyname
                	std::stringstream ss_bid;
                	std::stringstream ss_bid_hour;
					ss_bid<<"hincrby rts_ad_exp_req_pv:";
					ss_bid_hour<<"hincrby rts_ad_exp_req_pv:";
					std::string date;
					util::Func::get_time_str(&date, "%Y%m%d");
					std::string date_hour;
					util::Func::get_time_str(&date_hour, "%Y%m%d%H");
					ss_bid<<date<<"0000:1-D:source";
					ss_bid_hour<<date_hour<<"00:1-D:source";
					ss_bid<<" ";
					ss_bid_hour<<" ";
					//field_name;
					ss_bid<<"bid_pv:"<<source;
					ss_bid_hour<<"bid_pv:"<<source;
					ss_bid<<" ";
					ss_bid_hour<<" ";
					ss_bid<<count;
					ss_bid_hour<<count;
					cmd_bid=ss_bid.str();
					cmd_bid_hour=ss_bid_hour.str();
					LOG_DEBUG("redis cmd_bid[%s]", cmd_bid.c_str());
					LOG_DEBUG("redis cmd_bid_hour[%s]", cmd_bid_hour.c_str());
                }
                //step 4:执行redis命令
                if(redis_context_==NULL)
                {
                    rt=redis_connect();
                    if(rt != 0)
                    {
                        break;
                    }
                }

                redisReply * r =(redisReply *)redisCommand(redis_context_, cmd.c_str());
                redisReply * r_hour =(redisReply *)redisCommand(redis_context_, cmd_hour.c_str());
                if(bid_type ==1) {
                	redisReply * r_bid =(redisReply *)redisCommand(redis_context_, cmd_bid.c_str());
                	redisReply * r_bid_hour =(redisReply *)redisCommand(redis_context_, cmd_bid_hour.c_str());
					if(r==NULL) {
						LOG_ERROR("redis execute error, cmd[%s]", cmd.c_str());
						redisFree(redis_context_);
						redis_context_=NULL;
						redis_connect();
						break;
					}	else {
						freeReplyObject(r);
					}
					if(r_hour==NULL) {
						LOG_ERROR("redis execute error, cmd[%s]",cmd_hour.c_str());
						redisFree(redis_context_);
						redis_context_=NULL;
						redis_connect();
						break;
					} else {
						freeReplyObject(r_hour);
					}
					if(r_bid==NULL) {
						LOG_ERROR("redis execute error, cmd[%s]",cmd_bid.c_str());
						redisFree(redis_context_);
						redis_context_=NULL;
						redis_connect();
						break;
					} else {
						freeReplyObject(r_bid);
					}
					if(r_bid_hour==NULL) {
						LOG_ERROR("redis execute error, cmd[%s]",cmd_bid_hour.c_str());
						redisFree(redis_context_);
						redis_context_=NULL;
						redis_connect();
						break;
					} else {
						freeReplyObject(r_bid_hour);
					}

                } else {
					if(r==NULL) {
						LOG_ERROR("redis execute error, cmd[%s]", cmd.c_str());
						redisFree(redis_context_);
						redis_context_=NULL;
						redis_connect();
						break;
					} else {
						freeReplyObject(r);
					}
					if(r_hour==NULL) {
						LOG_ERROR("redis execute error, cmd[%s]", cmd_hour.c_str());
						redisFree(redis_context_);
						redis_context_=NULL;
						redis_connect();
						break;
					} else {
						freeReplyObject(r_hour);
					}
                }
            }
            source_map.clear();
            source_map[now]=1;
        }
    }while(0);
    return rt;
}

int ProcessStat::stat_source_var_exp_id(const std::string & source,const std::string & exp_id)
{
    int rt=0;
    do{
        time_t now=time(NULL);
        std::string str = source;
        str.append("`");
        str.append(exp_id);
        std::map<time_t, int>& source_map=sources_var_exp_st_all[str];
//        std::pair<const std::string, time_t> pair=std::make_pair(source, now);
        if(source_map.count(now) > 0)
        {
            source_map[now]++;
        }else
        {
            std::map<time_t, int>::iterator it;
            for(it=source_map.begin(); it != source_map.end(); it++)
            {
                int count=it->second;
                std::stringstream ss;
                std::stringstream ss_hour;
                //INCRBY KEY_NAME FIELD_NAME INCR_BY_NUMBER
                //keyname
                ss<<"hincrby rts_ad_exp_req_pv:";
                ss_hour<<"hincrby rts_ad_exp_req_pv:";
                std::string date;
                util::Func::get_time_str(&date, "%Y%m%d");
                std::string date_hour;
				util::Func::get_time_str(&date_hour, "%Y%m%d%H");
                ss<<date<<"0000:1-D:source`exp_id";
                ss_hour<<date_hour<<"00:1-D:source`exp_id";
                ss<<" ";
                ss_hour<<" ";
                //field_name;
                ss<<"req_pv:"<<source;
                ss_hour<<"req_pv:"<<source;
                ss<<"`"<<exp_id;
                ss_hour<<"`"<<exp_id;
                ss<<" ";
                ss_hour<<" ";
                ss<<count;
                ss_hour<<count;
                std::string cmd=ss.str();
                std::string cmd_hour=ss_hour.str();
                LOG_DEBUG("redis cmd[%s]", cmd.c_str());
                LOG_DEBUG("redis cmd_hour[%s]", cmd_hour.c_str());
                //step 4:执行redis命令
                if(redis_context_==NULL)
                {
                    rt=redis_connect();
                    if(rt != 0)
                    {
                        break;
                    }
                }
                redisReply * r =(redisReply *)redisCommand(redis_context_, cmd.c_str());
                redisReply * r_hour =(redisReply *)redisCommand(redis_context_, cmd_hour.c_str());
				if(r==NULL) {
					LOG_ERROR("redis execute error, cmd[%s]", cmd.c_str());
					redisFree(redis_context_);
					redis_context_=NULL;
					redis_connect();
					break;
				} else {
					freeReplyObject(r);
				}
				if(r_hour==NULL) {
					LOG_ERROR("redis execute error, cmd[%s]", cmd_hour.c_str());
					redisFree(redis_context_);
					redis_context_=NULL;
					redis_connect();
					break;
				} else {
					freeReplyObject(r_hour);
				}
            }
            source_map.clear();
            source_map[now]=1;
        }
    }while(0);
    return rt;
}

int ProcessStat::stat_deal_req_pv(const std::string & source, const std::string & deal_id, int req_ad_num)
{
    int rt=0;
    do{
        time_t now=time(NULL);
        DealReqInfo & deal_req_info=map_req_pv_[source];

        std::map<time_t, int>& deal_req_pv=deal_req_info.deal_req_pv_[deal_id];

        if(deal_req_pv.count(now) > 0)
        {
            deal_req_pv[now]+=req_ad_num;
        }else
        {
            std::map<time_t, int>::iterator it;
            for(it=deal_req_pv.begin(); it!=deal_req_pv.end(); it++)
            {
                int count=it->second;//次数
                std::stringstream ss;

                //INCRBY KEY_NAME FIELD_NAME INCR_BY_NUMBER
                //keyname
                ss<<"hincrby rts_ad_deal_req_pv:";
                std::string date;
                util::Func::get_time_str(&date, "%Y%m%d");
                ss<<date<<"0000:1-D:deal_id`source";

                ss<<" ";
                //field_name;
                ss<<"deal_req_pv:"<<deal_id<<"`"<<source;

                ss<<" ";
                ss<<count;

                std::string cmd=ss.str();
                LOG_DEBUG("redis cmd[%s]", cmd.c_str());


                //step 4:执行redis命令
                if(redis_context_==NULL)
                {
                    rt=redis_connect();
                    if(rt != 0)
                    {
                        break;
                    }
                }
              
                MON_ADD(ATTR_REDIS_WRITE, 1);          
                redisReply * r =(redisReply *)redisCommand(redis_context_, cmd.c_str());
                if(r==NULL)
                {
                    LOG_ERROR("redis execute error, cmd[%s]", cmd.c_str());
                    MON_ADD(ATTR_REDIS_WRITE_ERR, 1);          
                    redisFree(redis_context_);
                    redis_context_=NULL;
                    redis_connect();
                    break;
                }else
                {
                    freeReplyObject(r);
                }
            }
            deal_req_pv.clear();
            deal_req_pv[now]=req_ad_num;
        }
    }while(0);
    return rt;
}

/**
 * @brief           处理单个记录
 **/
int ProcessStat::proc(const char * buf, int buflen)
{
    int rt=0;
    do{
        std::map<std::string, std::string> mapkv;
        std::string input;
        input.assign(buf, buflen);
        util::UtilStr::inst(input).split_to_map("`", "=", mapkv);

        if(mapkv.count("source") == 0)
        {
            break;
        }
        std::string source=mapkv["source"];
        
        int req_ad_num=1;
        if(mapkv.count("req_ad_num") > 0)
        {
            req_ad_num=util::Func::to_int(mapkv["req_ad_num"]);
        }

        stat_source_pv(source);

        stat_source_ad_pv(source, req_ad_num);

        if(mapkv.count("deal_id") > 0)
        {
            stat_deal_req_pv(source, mapkv["deal_id"], req_ad_num);
        }
        
        if(mapkv.count("exp_id") > 0)
		{
        	std::vector<std::string> vr_exp_id=util::UtilStr::inst(mapkv["exp_id"]).split("|");
        	for(int i=0;i<vr_exp_id.size();i++) {
        	if(!vr_exp_id[i].empty()) {
        		if(!(strncmp(buf, "pv_status=0`", 12)!=0)) {
					stat_source_exp_id_pv(source,vr_exp_id[i],1);
				}
					stat_source_var_exp_id(source,vr_exp_id[i]);
        		}
        	}
        	if(!(strncmp(buf, "pv_status=0`", 12)!=0)) {
        		stat_source_exp_id_bid_all_pv(source,1);
        	} else {
        		stat_source_exp_id_all_pv(source,0);
        	}
		}

        //在monitor上画source监控图
        report_source(source);

        if(mapkv.count("view_type") != 0)
        {
            report_view_type(mapkv["view_type"]);
        }

        //step 1:过滤不竞价的
        if(strncmp(buf, "pv_status=0`", 12)!=0)
        {//不出价的不管
            break;
        }


        //step 2:获取创意和来源
#if 0
        std::string cid;
        std::string source;
        rt=get_cid_and_source(buf, buflen, cid, source);
        if(rt != 0)
        {
            break;
        }
#endif

        std::string campaign_id=mapkv["campaign_id"];
        std::string ad_id=mapkv["ad_id"];
        std::string bid_ad_num="1";
        if(mapkv.count("bid_ad_num") > 0)
        {
            bid_ad_num=mapkv["bid_ad_num"];
        }

        int bid_ad_cnt=util::Func::to_int(bid_ad_num);

        std::vector<std::string> vr_ad=util::UtilStr::inst(ad_id).split("|");
        std::vector<std::string> vr_camp=util::UtilStr::inst(campaign_id).split("|");

        int ad_size=vr_ad.size();
        int camp_size=vr_camp.size();
        int ad_count;
        if(bid_ad_cnt <= ad_size && bid_ad_cnt <= camp_size)
        {
            ad_count=bid_ad_cnt;
        }else
        {
            ad_count=ad_size<camp_size?ad_size:camp_size;
        }
        LOG_DEBUG("bid_ad_cnt[%d]ad_size[%d]camp_size[%d]ad_count[%d]ad_id[%s]campaign_id[%s]\n", bid_ad_cnt, ad_size, camp_size, ad_count, ad_id.c_str(), campaign_id.c_str());

        for(int i=0; i<ad_count; i++)
        {
            if(util::Func::to_int(vr_ad[i])==0 || util::Func::to_int(vr_camp[i])==0 )
            {
                continue;
            }

            //step 3:拼redis命令行
            std::string cmd="hincrby ";
         
            //INCRBY KEY_NAME FIELD_NAME INCR_BY_NUMBER
            std::string key_name;
            key_name="rts_ad_bid_pv:";
            std::string date;
            util::Func::get_time_str(&date, "%Y%m%d");
            key_name+=date+"0000:1-D:ad_id`campaign_id`source";
         
            std::string field_name;
            field_name="bid_pv:";
            field_name+=vr_ad[i]+"`"+vr_camp[i]+"`"+source;
         
            cmd+=key_name+" "+field_name+" 1";
         
            LOG_DEBUG("redis cmd[%s]", cmd.c_str());
            
         
            //step 4:执行redis命令
            if(redis_context_==NULL)
            {
                rt=redis_connect();
                if(rt != 0)
                {
                    break;
                }
            }
         
            MON_ADD(ATTR_REDIS_WRITE, 1);          
            redisReply * r =(redisReply *)redisCommand(redis_context_, cmd.c_str());
            if(r==NULL)
            {
                MON_ADD(ATTR_REDIS_WRITE_ERR, 1);          
                LOG_ERROR("redis execute error, cmd[%s]", cmd.c_str());
                MON_ADD(ATTR_REDIS_WRITE_ERR, 1);          
                redisFree(redis_context_);
                redis_context_=NULL;
                redis_connect();
                break;
            }else
            {
                freeReplyObject(r);
            }

            if(mapkv.count("deal_id")>0)
            {
                //step 3:拼redis命令行
                std::string cmd="hincrby ";
                
                //INCRBY KEY_NAME FIELD_NAME INCR_BY_NUMBER
                std::string key_name;
                key_name="rts_ad_deal_bid_pv:";
                std::string date;
                util::Func::get_time_str(&date, "%Y%m%d");
                key_name+=date+"0000:1-D:campaign_id`deal_id`source";
                
                std::string field_name;
                field_name="deal_bid_pv:";
                field_name+=vr_camp[i]+"`"+mapkv["deal_id"]+"`"+source;
                
                cmd+=key_name+" "+field_name+" 1";
                
                LOG_DEBUG("redis cmd[%s]", cmd.c_str());
                
                
                //step 4:执行redis命令
                if(redis_context_==NULL)
                {
                    rt=redis_connect();
                    if(rt != 0)
                    {
                        break;
                    }
                }
                                
                MON_ADD(ATTR_REDIS_WRITE, 1);          
                redisReply * r =(redisReply *)redisCommand(redis_context_, cmd.c_str());
                if(r==NULL)
                {
                    LOG_ERROR("redis execute error, cmd[%s]", cmd.c_str());
                    MON_ADD(ATTR_REDIS_WRITE_ERR, 1);          
                    redisFree(redis_context_);
                    redis_context_=NULL;
                    redis_connect();
                    break;
                }else
                {
                    freeReplyObject(r);
                }
            }

        }
    }while(0);
    return rt;
}

int ProcessStat::get_cid_and_source(const char * buf, int buflen, std::string &cid, std::string &source)
{
    int rt=0;
    do{
        std::string pvlog(buf, buflen);
        size_t pos=pvlog.find("`source=");
        if(pos != std::string::npos)
        {
            int offset=strlen("`source=");
            int endpos=pvlog.find("`", pos+offset);
            source=pvlog.substr(pos+offset, endpos-pos-offset);
        }
        pos=pvlog.find("`creative_id=");
        if(pos != std::string::npos)
        {
            int offset=strlen("`creative_id=");
            int endpos=pvlog.find("`", pos+offset);
            cid=pvlog.substr(pos+offset, endpos-pos-offset);
        }
    }while(0);
    return rt;
}

int ProcessStat::get_value(const char * buf, int buflen, const char * key, std::string & val)
{
    int rt=0;
    do{
        std::string pvlog(buf, buflen);
        char find_text[128];
        snprintf(find_text, 128, "`%s=", key);
        size_t pos=pvlog.find(find_text);
        if(pos != std::string::npos)
        {
            int offset=strlen(find_text);
            int endpos=pvlog.find("`", pos+offset);
            val=pvlog.substr(pos+offset, endpos-pos-offset);
        }else
        {
            return -1;
        }
    }while(0);
    return rt;
}
void ProcessStat::report_source(const std::string & source)
{
    int isource=util::Func::to_int(source);
    if(isource > 0 && isource <= MAX_SOURCE)
    {
        MON_ADD(SOURCE_0+isource, 1);
    }
}

void ProcessStat::report_view_type(const std::string & vt)
{
    int ivt=util::Func::to_int(vt);
    if(ivt > 0 && ivt <= MAX_VIEW_TYPE)
    {
        MON_ADD(VIEW_TYPE_0+ivt, 1);
    }
}

}//log
}//poseidon

