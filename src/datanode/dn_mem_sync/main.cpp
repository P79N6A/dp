/**
 **/

//#include "config.h"
#include "protocol/src/poseidon_proto.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <iostream>
#include "ha.h"
#include "util/util_str.h"
#include "util/func.h"
#include "comm_event.h"
#include "comm_event_interface.h"
#include "comm_event_factory.h"
#include "gflags/gflags.h"
#include "data_api/kv_reg_mgr.h"
#include "gflags/gflags.h"
#include <fstream>
#include "json/json.h"
#include "monitor_api.h"
using namespace poseidon::mem_sync;
using namespace poseidon::dn;

DEFINE_string(c_file, "creative_file","creative_file");
DEFINE_string(zk, "127.0.0.1:2181","zk_iplist");
DEFINE_string(dsip,"10.32.50.214", "data_server ip");
DEFINE_int32(dsport, 20000, "data_server port");

#define ATTR_KV_PUT_ERROR	2365
#define ATTR_KV_REG_ERROR	2366

void usage(char * basename)
{
    fprintf(stderr, "Usage:%s config_file", basename);
}

int parse_line(char * buf,poseidon::common::Creative & creative)
{
	Json::Reader reader_;
    int rt=0;
    do{
        try
        {
            Json::Value root;
            if(!reader_.parse(buf, root, false))
            {
//                LOG_ERROR("json parse error");
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
            //LOG_ERROR("get exception[%s]buf[%s]\n", e.what(), buf);
        }catch(...)
        {
            //LOG_ERROR("get unknowned exception");
        }
    }while(0);
    return rt;
}

int main(int argc, char * argv[])
{

	int rt = 0, n = 10, data_id = 31;
	google::SetVersionString("1.0.0");
	google::ParseCommandLineFlags(&argc, &argv, true);
    char buf[10240];                //临时保存读取出来的文件内容
    string message;
    string* result;
    char *file=new char[strlen(FLAGS_c_file.c_str())+1];
    strcpy(file, FLAGS_c_file.c_str());
    ifstream in(file);
    string s;
    KVRegMgr &mgr = KVRegMgr::get_mutable_instance();
    mgr.init(FLAGS_zk, FLAGS_dsip, FLAGS_dsport);
    while(getline(in,s)) {
    	char *buf = new char[strlen(s.c_str())+1];
    	strcpy(buf, s.c_str());
    	poseidon::common::Creative creative;
    	parse_line(buf,creative);
    	stringstream stream;
		stream<<creative.creative_id();
//		cout<<stream.str()<<endl;
		rt = mgr.put(stream.str(), s.c_str());
		if(rt != 0) {
			MON_ADD(ATTR_KV_PUT_ERROR, 1);
		}
    }
    rt = mgr.reg_data(data_id);
    if(rt != 0) {
    	MON_ADD(ATTR_KV_REG_ERROR, 1);
    }
    return rt;
}
