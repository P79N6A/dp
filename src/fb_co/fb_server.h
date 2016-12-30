/*
 * fb_server.h
 * Created on: 2016-12-16
 */

#ifndef FB_CO_FB_SERVER_H_
#define FB_CO_FB_SERVER_H_

#include <string>
#include <map>

#include "co2/timer/timer.h"
#include "co2/server/udp_server.h"

#include "fb_co/cache.h"
#include "fb_co/redis/redis.h"

#include "protocol/src/poseidon_proto.h"

namespace poseidon {
namespace feedback {

using co2::server::UDPServer;

class SessData;

class FBServer : public UDPServer {
public:
    FBServer();
    virtual ~FBServer();

    int Init(); /* init redis pool, ... etc. */
    void OnTimer(void *arg);

    virtual int OnDatagram(const std::string &host, int port,
        const char *buf, int size);

    co2::redis::RedisPool &GetRedisPool(void) { return pool_; }
private:
    // int ReqAdvertiseNum(std::string &cmd, SessData *);
    int ReqAdvertiseCreative(uint64_t creative_id, std::string &cmd, SessData *);
    int ReqAdvertiseGroup(uint32_t group_id, std::string &cmd, SessData *);
    int ReqAdvertiseCampaign(uint32_t campaign_id, std::string &cmd, SessData *);
    int ReqAdvertiser(uint32_t advertiser_id, const std::string &lastday,
        std::string &cmd, std::string &yesCmd, SessData *);

    int ReqPdbDeal(const std::string &deal_id, int campaign_id,
        std::string &dealCmd, std::string &campaignCmd, SessData *);

    int OnAdvertiseNum(redisReply *, SessData *);
    int OnAdvertiseCreative(redisReply *, SessData *);
    int OnAdvertiseGroup(redisReply *, SessData *);
    int OnAdvertiseCampaign(redisReply *, SessData *);
    int OnAdvertiser(redisReply *, SessData *);

    int OnLastDayAdvertiser(const std::string &dt, redisReply *, SessData *);
    int OnPdbDeal(redisReply *reply, SessData *);
    int OnPdbDealCampaign(redisReply *reply, SessData *);

    int Response(SessData * sess);
private:
    co2::redis::RedisPool pool_;
    co2::timer::Timer *timer_;

    /* cache lastday's advertiser data */
    std::map<std::string, std::map<std::string, std::string> > lastdayAdvertiserMap_;

    Cache<std::string, std::string> advertiser_cache_;
    Cache<std::string, std::string> campaign_cache_;
    Cache<std::string, std::string> group_cache_;
    Cache<std::string, std::string> creative_cache_;
    Cache<std::string, common::FeedbackInfo_PdbFeedback> deal_cache_;
    Cache<std::string, common::FeedbackInfo_PdbFeedback> deal_campaign_cache_;
    Cache<std::string, std::string> count_cache_;
};


}  // namespace feedback
}  // namespace poseidon



#endif /* FB_CO_FB_SERVER_H_ */
