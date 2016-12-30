/**
 **/

#ifndef  _FB_SERVER_H_
#define  _FB_SERVER_H_

#include <boost/serialization/singleton.hpp>
#include "comm_event.h"
#include "comm_event_interface.h"
#include "comm_event_factory.h"
#include "protocol/src/poseidon_proto.h"
#include "redis_access.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <set>
#include <map>
#include "json/json.h"


namespace poseidon
{
namespace feedback
{

enum
{
	MEMCAMPAIGN=1,
	ADKEY=(1<<1),
	ADVERTISTER=(1<<2),
	ADNUM=(1<<3),
	CREATIVE=(1<<4),
	DEAL_ID=(1<<5),
	DEAL_CAMPAIGN=(1<<6),
	YESTERDAYCOST=(1<<7),
};

struct SessData
{
	FeedbackRequest fbreq;						//请求包
	FeedbackResponse fbrsp;						//响应包
//    common::FeedbackInfo feedbackInfo;			//
    common::FeedbackInfo::CreativeCost creativeCost;
    std::vector<std::string> countVt;
    std::vector<std::string> memCampaignVt;
    std::vector<std::string> advertisterVt;
    std::vector<std::string> adVt;
    std::vector<std::string> creativeVt;
    std::vector<std::string> dealVt;
    std::vector<std::string> deal_campaignVt;
    std::vector<std::string> yesterday_CostVt;
	std::map<std::string,std::string> memCampaignMap;
	std::map<std::string,std::string> advertisterMap;
	std::map<std::string,std::string> adMap;
	std::map<std::string,std::string> creativeMap;
	std::map<std::string,std::string> countMap;
	std::map<std::string,std::string> adNum;
    std::map<std::string,common::FeedbackInfo_PdbFeedback> dealMap;
    std::map<std::string,common::FeedbackInfo_PdbFeedback> deal_campaignMap;
    struct sockaddr_in addr;                    //地址
    int status;                                 //状态
    std::string str;
    int num;
};

class FbServer:public dc::common::comm_event::CommBase, public boost::serialization::singleton<FbServer>
{
public:
    /**
     * @brief               process req package
     **/
    virtual int handle_read(const char * buf, const int len, struct sockaddr_in & client_addr);

    int reply_client(SessData * sess);

    int send_get_campaign(SessData * sess,std::string cmd);

    int send_get_adgroup(SessData * sess,std::string cmd);

    int send_get_count(SessData * sess,std::string cmd);

    int send_get_advertister(SessData * sess,std::string cmd);

    int send_get_creative(SessData * sess,std::string cmd);

    int send_get_adNum(SessData * sess,std::string cmd);

    int send_get_deal(SessData * sess,std::string cmd);

    int send_get_deal_campaign(SessData * sess,std::string cmd);

    int send_get_yesterdayCost(SessData * sess,std::string cmd);

    int on_get_adgroup(void * sess, void * reply , int err_code, const char * err_str);

    int on_get_campaign(void * sess, void * reply , int err_code, const char * err_str);

    int on_get_count(void * sess, void * reply , int err_code, const char * err_str);

    int on_get_advertister(void * sess, void * reply , int err_code, const char * err_str);

    int on_get_creative(void * sess, void * reply , int err_code, const char * err_str);

    int on_get_deal_campaign(void * data, void * reply, int err_code, const char * err_str );

    int on_get_deal(void * data, void * reply, int err_code, const char * err_str );

    int on_get_yesterdayCost(void * data, void * reply, int err_code, const char * err_str );

    int getFromRedis(void * data, void * reply, int err_code, const char * err_str,int status);
    int proc_sess(SessData * sess);

    SessData * alloc_sess()
    {
        SessData * p=NULL;
        p=new(std::nothrow) SessData();
        if(p != NULL)
        {
            sess_.insert(p);
            p->status=0;
            return p;
        }else
        {
            return NULL;
        }
    }
    void free_sess(SessData * sess)
    {
        if(sess_.count(sess) > 0)
        {
            delete sess;
            sess_.erase(sess);
        }
    }
    bool sess_valid(SessData * sess)
    {
        return sess_.count(sess)>0;
    }
    static int s_on_get_adgroup(void * data, void * reply, int err_code, const char * err_str);

    static int s_on_get_campaign(void * data, void * reply, int err_code, const char * err_str);

    static int s_on_get_count(void * data, void * reply, int err_code, const char * err_str);

    static int s_on_get_advertister(void * data, void * reply, int err_code, const char * err_str);

    static int s_on_get_creative(void * data, void * reply, int err_code, const char * err_str);

    static int s_on_get_deal(void * data, void * reply, int err_code, const char * err_str );

    static int s_on_get_deal_campaign(void * data, void * reply, int err_code, const char * err_str );

    static int s_on_get_yesterdayCost(void * data, void * reply, int err_code, const char * err_str );

private:
    std::set<SessData *> sess_;
    Json::Reader reader_;
    std::map<std::string, std::string> yesterday_Costmap_;
};

}
}


#endif   // ----- #ifndef _dn_SERVER_H_  -----



