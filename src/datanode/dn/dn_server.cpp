/**
 **/

#include "dn_server.h"
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
#include <stdio.h>
#include <cstdlib>
#include <cstdio>
#include "data_api/kv_api.h"

using namespace std;
using namespace poseidon::mem_sync;
namespace poseidon
{
namespace dn
{

/**
 * @brief               process req package
 **/

/**
 * @brief           初始化
 **/
int DnServer::init()
{
    int rt=0;
    do{
    	ka = new KVApi();
    	ka->init();
    	data_id = 31;
//        redis_host_=Config::get_mutable_instance().get_redis_host();
//        redis_port_=Config::get_mutable_instance().get_redis_port();
//        rt=redis_connect();
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
int DnServer::redis_connect()
{
    int rt=0;
//    do{
//
//        redis_context_=redisConnect(redis_host_.c_str(), redis_port_);
//        if(redis_context_->err)
//        {
//            LOG_ERROR("redisConnect error[%s]\n", redis_context_->errstr);
////            MON_ADD(ATTR_REDIS_WRITE_ERR, 1);
//            redisFree(redis_context_);
//            redis_context_=NULL;
//            rt=-1;
//        }
//    }while(0);
    return rt;
}

int DnServer::handle_read(const char * buf, const int len, struct sockaddr_in & client_addr)
{

    MON_ADD(ATTR_DN_REQ, 1);
    int rt=0;
    SessData * sess=NULL;
    do{
        sess = alloc_sess();
//        LOG_INFO("init DNServer instance handle_read");
        if(sess == NULL )
        {
            LOG_ERROR("new SessData return NULL");
            rt=-1;
            break;
        }
//        if(sess->dnreq.creative_ids_size()>10)
//		{
//			LOG_ERROR("new SessData return NULL");
//			rt=-1;
//			break;
//		}
        memcpy(&(sess->addr), &client_addr, sizeof(struct sockaddr_in) );
        DNRequest & dnreq=sess->dnreq;
        DNResponse & dnrsp=sess->dnrsp;
        
        if(!dnreq.ParseFromArray(buf, len))
        {
            MON_ADD(ATTR_PARSE_ERR, 1);
            LOG_ERROR("new SessData return NULL");
            dnrsp.set_error_code(common::ERROR_NO_RESULT);
			reply_client(sess);
            rt=-1;
            break;
        }
        LOG_DEBUG("pid[%d]dnreq[%s]", getpid(), dnreq.DebugString().c_str());

        if(dnreq.creative_ids_size()==0)
        {
        	MON_ADD(ATTR_DN_REQ, 1);
        	dnrsp.set_error_code(common::ERROR_NO_RESULT);
        	reply_client(sess);
        	rt=-1;
        	break;
        }
//        sess->creatives = "hmget creative";
//		for(int i=0;i<sess->dnreq.creative_ids().size();i++) {
//			sess->creatives.append(" ");
//			sess->creatives.append(util::Func::to_str(dnreq.creative_ids().Get(i)));
//			LOG_DEBUG("trace_id[%s]creative_id[%s]",dnreq.trace_id().c_str(),util::Func::to_str(dnreq.creative_ids().Get(i)));
//		}
		rt=send_get_Creative(sess);
        if(rt != 0)
        {
            LOG_ERROR("send_get_creative error[%d]", rt);
            LOG_DEBUG("redis error noResult tranc_id[%s]",dnreq.trace_id());
            dnrsp.set_error_code(common::ERROR_NO_RESULT);
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


int DnServer::reply_client(SessData * sess)
{
    int rt=0;
    do{
        DNResponse & dnrsp=sess->dnrsp;
        DNRequest & dnreq=sess->dnreq;
        if(dnreq.has_session_id())
        {
            dnrsp.set_session_id(dnreq.session_id());
        }
        if(dnreq.has_trace_id())
        {
            dnrsp.set_trace_id(dnreq.trace_id());
        }

        char hostname[256];
        memset(hostname, 0x00, 256);
        if( gethostname(hostname, 256) == 0 )
        {
            dnrsp.set_hostname(hostname);
        }

        std::string sendbuf;
        if(!dnrsp.SerializeToString(&sendbuf))
        {
            MON_ADD(ATTR_PACK_ERROR, 1);
            LOG_ERROR("SerializeToString error trance_id[%s]",dnreq.trace_id());
            rt=-1;
            break;
        }
        LOG_DEBUG("dnrsp[%s]\n", dnrsp.trace_id().c_str());
        LOG_DEBUG("dnrsp[%s]\n", dnrsp.DebugString().c_str());

        if(!Config::get_mutable_instance().is_dumb())
        {
            send_pkg(sendbuf.c_str(), sendbuf.length(), sess->addr);
            LOG_DEBUG("dnrsp[%s]\n", dnrsp.DebugString().c_str());
        }
        MON_ADD(ATTR_DN_RSP, 1);
    }while(0);
    free_sess(sess);
    LOG_DEBUG("");
    return rt;
}

int DnServer::send_get_Creative(SessData * sess)
{
    int rt=0;
//    do{
//
//        LOG_DEBUG("cmds[%s] tranceid[%s]", sess->creatives.c_str(),sess->dnreq.trace_id().c_str());
//		//step 4:执行redis命令
//		if(redis_context_==NULL)
//		{
//			rt=redis_connect();
//			if(rt != 0)
//			{
//				break;
//			}
//		}
//
//		redisReply * pReply =(redisReply *)redisCommand(redis_context_, sess->creatives.c_str());
//		if(pReply==NULL)
//		{
////			LOG_ERROR("redis execute error, cmd[%s]", cmd.c_str());
////			MON_ADD(ATTR_REDIS_WRITE_ERR, 1);
//			redisFree(redis_context_);
//			redis_context_=NULL;
//			redis_connect();
//			break;
//		}
//		if(pReply->type != REDIS_REPLY_ARRAY)
//		{
//			if(pReply->type == REDIS_REPLY_NIL)
//			{
//				MON_ADD(ATTR_CREATIVE_NO_KEY, 1);
//				LOG_DEBUG("ATTR_CREATIVE_NO_KEY trancid[%s]\n",sess->dnreq.trace_id());
//				LOG_ERROR("ATTR_CREATIVE_NO_KEY trancid[%s]\n",sess->dnreq.trace_id());
//			}
//			if(pReply->type == REDIS_REPLY_ERROR)
//			{
//				LOG_DEBUG("on_get_creative->type != STRING[%d][%s] trancid[%s]", pReply->type, pReply->str,sess->dnreq.trace_id().c_str());
//				LOG_ERROR("on_get_creative->type != STRING[%d][%s]", pReply->type, pReply->str);
//			}
//			LOG_DEBUG("on_get_creative->type != STRING[%d]", pReply->type);
//		} else
//		{
//			LOG_DEBUG("on_get_Creative1");
//			poseidon::common::Creative* creative;
//			if (pReply->type == REDIS_REPLY_ARRAY) {
//					for (int i = 0; i < pReply->elements; i++) {
//					redisReply* childReply = pReply->element[i];
//					LOG_DEBUG("on_get_creative trancid[%s]",sess->dnreq.trace_id().c_str());
//					if(childReply->type == REDIS_REPLY_STRING) {
//						parse_line(childReply->str,sess->creative);
//						LOG_DEBUG("on_get_creative->type != STRING[%s] trancid[%s]",childReply->str,sess->dnreq.trace_id().c_str());
//						if((sess->creative.status()==3) || (sess->creative.status()==4) ) {
//							creative = sess->dnrsp.add_creative();
//							creative->CopyFrom(sess->creative);
//							sess->creative.Clear();
//						} else {
//							MON_ADD(ATTR_CREATIVE_STATUS_ERROR, 1);
//						}
//					}
//				}
//			}
//		}
//		if(pReply!=NULL)
//		{
//			freeReplyObject(pReply);
//		}
//    }while(0);
    get_Creative_mem(sess);
}

int DnServer::get_Creative_mem(SessData * sess)
{
    int rt=0;
    do{
        LOG_DEBUG("cmds[%s] tranceid[%s]", sess->creatives.c_str(),sess->dnreq.trace_id().c_str());

		string val;
		for(int i=0;i<sess->dnreq.creative_ids_size();i++) {
			poseidon::common::Creative* creative;
			int c_id = sess->dnreq.creative_ids(i);
			rt = ka->get(data_id, util::Func::to_str(c_id), val);
			if(rt == 0) {
//				char *buf=new char[strlen(val.c_str())+1];
//				strcpy(buf, val.c_str());

				LOG_DEBUG("buf[%s]",val.c_str());
				parse_line(val.c_str(),sess->creative);
				LOG_DEBUG("on_get_creative->type != STRING[%s] trancid[%s]",val.c_str(),sess->dnreq.trace_id().c_str());
				if((sess->creative.status()==3) || (sess->creative.status()==4) ) {
					creative = sess->dnrsp.add_creative();
					creative->CopyFrom(sess->creative);
//					creative->Clear();
				} else {
					MON_ADD(ATTR_CREATIVE_STATUS_ERROR, 1);
				}
				sess->creative.Clear();
			}
		}
    }while(0);
    sess->status =0;
    rt=proc_sess(sess);
    return rt;
}



//int DnServer::on_get_Creative(void * data, void * reply, int err_code, const char * err_str )
//{
//    int rt=0;
//    SessData * sess=(SessData *)data;
//    LOG_DEBUG("sess1 taracid[%s]", sess->dnreq.trace_id().c_str());
//    redisReply * pReply=(::redisReply *)reply;
//    do{
//        if(!sess_valid(sess))
//        {
//            MON_ADD(ATTR_SESS_INVALID, 1);
//            LOG_DEBUG("sess invalid[%d]", getpid());
//            rt=-1;
//            break;
//        }
//        if(err_code != 0)
//        {
//            LOG_ERROR("on_get_Creative error[%d][%s]\n", err_code, err_str);
//            LOG_DEBUG("on_get_Creative error[%d][%s]\n", err_code, err_str);
//            LOG_ERROR("on_get_Creative error[%d][%s]trancid[%s]\n", err_code, err_str,sess->dnreq.trace_id());
//        }else if(pReply == NULL )
//        {
//            LOG_ERROR("pReply == NULL");
//        }else if(pReply->type != REDIS_REPLY_ARRAY)
//        {
//            if(pReply->type == REDIS_REPLY_NIL)
//            {
//                MON_ADD(ATTR_CREATIVE_NO_KEY, 1);
//                LOG_DEBUG("ATTR_CREATIVE_NO_KEY trancid[%s]\n",sess->dnreq.trace_id());
//                LOG_ERROR("ATTR_CREATIVE_NO_KEY trancid[%s]\n",sess->dnreq.trace_id());
//            }
//            if(pReply->type == REDIS_REPLY_ERROR)
//            {
//            	LOG_DEBUG("on_get_creative->type != STRING[%d][%s] trancid[%s]", pReply->type, pReply->str,sess->dnreq.trace_id().c_str());
//            	LOG_ERROR("on_get_creative->type != STRING[%d][%s]", pReply->type, pReply->str);
//            }
//            LOG_DEBUG("on_get_creative->type != STRING[%d]", pReply->type);
//        } else
//        {
//        	LOG_DEBUG("on_get_Creative1");
//			poseidon::common::Creative* creative;
//			if (pReply->type == REDIS_REPLY_ARRAY) {
//					for (int i = 0; i < pReply->elements; i++) {
//					redisReply* childReply = pReply->element[i];
//					LOG_DEBUG("on_get_creative trancid[%s]",sess->dnreq.trace_id().c_str());
//					if(childReply->type == REDIS_REPLY_STRING) {
//						parse_line(childReply->str,sess->creative);
//						LOG_DEBUG("on_get_creative->type != STRING[%s] trancid[%s]",childReply->str,sess->dnreq.trace_id().c_str());
//						if((sess->creative.status()==3) || (sess->creative.status()==4) ) {
//							creative = sess->dnrsp.add_creative();
//							creative->CopyFrom(sess->creative);
//							sess->creative.Clear();
//						} else {
//							MON_ADD(ATTR_CREATIVE_STATUS_ERROR, 1);
//						}
//					}
//				}
//			}
//		}
//		sess->status =0;
//       }while(0);
//	rt=proc_sess(sess);
//    return rt;
//}

int DnServer::parse_line(const char * buf,common::Creative & creative)
{
    int rt=0;
    do{
        try
        {
            Json::Value root;
            if(!reader_.parse(buf, root, false))
            {
                LOG_ERROR("json parse error");
                rt=-1;
                break;
            }
            if(!root["creative_id"].isNull())
            {
            	creative.set_creative_id(root["creative_id"].asInt());
            }
            if(!root["status"].isNull())
			{
				creative.set_status(root["status"].asInt());
			}
            if(!root["title"].isNull())
			{
				creative.set_title(root["title"].asCString());
			}
            if(!root["template_id"].isNull())
			{
				creative.set_template_id(root["template_id"].asInt());
			}
            if(!root["content"].isNull())
			{
				creative.set_content(root["content"].asCString());
			}
            if(!root["dest_url"].isNull())
			{
				creative.set_dest_url(root["dest_url"].asCString());
			}
            if(!root["click_url"].isNull())
			{
				creative.set_click_url(root["click_url"].asCString());
			}
            if(!root["creative_category"].isNull())
			{
            	if(root["creative_category"].isArray()) {
            		for(int i=0;i<root["creative_category"].size();i++) {
            			creative.add_creative_category(root["creative_category"][i].asInt());
            		}
            	}
			}
            if(!root["creative_level"].isNull())
			{
				creative.set_creative_level(root["creative_level"].asInt());
			}
            if(!root["creative_format"].isNull())
			{
				creative.set_creative_format(root["creative_format"].asInt());
			}
            if(!root["creative_brand_id"].isNull())
			{
				creative.set_creative_brand_id(root["creative_brand_id"].asInt());
			}
            if(!root["width"].isNull())
			{
				creative.set_width(root["width"].asInt());
			}
            if(!root["height"].isNull())
			{
				creative.set_height(root["height"].asInt());
			}
            if(!root["landing_mode"].isNull())
			{
				creative.set_landing_mode(root["landing_mode"].asInt());
			}
            if(!root["landing_url_target"].isNull())
			{
				creative.set_landing_url_target(root["landing_url_target"].asInt());
			}
            if(!root["subject_name"].isNull())
			{
				creative.set_subject_name(root["subject_name"].asString());
			}
            if(!root["subject_desc"].isNull())
			{
				creative.set_subject_desc(root["subject_desc"].asString());
			}
            if(!root["subject_category_id"].isNull())
			{
				creative.set_subject_category_id(root["subject_category_id"].asInt());
			}
            if(!root["app_os_platform"].isNull())
			{
				creative.set_app_os_platform(root["app_os_platform"].asInt());
			}
            if(!root["website_url"].isNull())
			{
				creative.set_website_url(root["website_url"].asString());
			}
            if(!root["website_name"].isNull())
			{
				creative.set_website_name(root["website_name"].asString());
			}
            if(!root["a_id"].isNull())
			{
				creative.set_a_id(root["a_id"].asInt());
			}
            if(!root["industry_id"].isNull())
			{
				creative.set_industry_id(root["industry_id"].asInt());
			}
            if(!root["plan_id"].isNull())
			{
				creative.set_plan_id(root["plan_id"].asInt());
			}
            if(!root["ad_type_id"].isNull())
			{
				creative.set_ad_type_id(root["ad_type_id"].asInt());
			}
            if(!root["ad_format_id"].isNull())
			{
				creative.set_ad_format_id(root["ad_format_id"].asInt());
			}
            if(!root["ad_name"].isNull())
			{
				creative.set_ad_name(root["ad_name"].asString());
			}
            if(!root["subject_id"].isNull())
			{
				creative.set_subject_id(root["subject_id"].asInt());
			}
            if(!root["download_type"].isNull())
			{
				creative.set_download_type(root["download_type"].asInt());
			}
            if(!root["open_type"].isNull())
			{
				creative.set_open_type(root["open_type"].asInt());
			}
            if(!root["deeplink_url"].isNull())
			{
				creative.set_deeplink_url(root["deeplink_url"].asString());
			}
            if(!root["ad_words"].isNull())
			{
				creative.set_ad_words(root["ad_words"].asString());
			}
            if(!root["img_url"].isNull())
			{
				creative.set_img_url(root["img_url"].asString());
			}
            if(!root["video_url"].isNull())
			{
				creative.set_video_url(root["video_url"].asString());
			}
            if(!root["suffix"].isNull())
			{
				creative.set_suffix(root["suffix"].asString());
			}
            if(!root["material_id"].isNull())
			{
				creative.set_material_id(root["material_id"].asString());
			}
            if(!root["specific_data"].isNull())
			{
				creative.set_specific_data(root["specific_data"].asString());
			}
            if(!root["ext_cid"].isNull())
			{
            	creative.set_ext_cid(root["ext_cid"].asString());
			}
        }catch(const std::exception & e)
        {
            LOG_ERROR("get exception[%s]buf[%s]\n", e.what(), buf);
        }catch(...)
        {
            LOG_ERROR("get unknowned exception");
        }
    }while(0);
    return rt;
}


int DnServer::proc_sess(SessData * sess)
{
    int rt=0;
    do{
    	DNRequest & dnreq=sess->dnreq;
    	DNResponse & dnrsp=sess->dnrsp;
        if( sess->status != 0 )
        {
        	dnrsp.set_error_code(common::ERROR_NO_RESULT);
        } else {
        	dnrsp.set_error_code(common::ERROR_NONE);
        }
        LOG_DEBUG("proc_sess");
        reply_client(sess);
    }while(0);
    return rt;
}

//int DnServer::s_on_get_Creative(void * data, void * reply, int err_code, const char * err_str )
//{
//    return DnServer::get_mutable_instance().on_get_Creative(data, reply, err_code, err_str);
//}

}
}
