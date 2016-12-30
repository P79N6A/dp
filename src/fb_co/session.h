/*
 * session.h
 * Created on: 2016-12-16
 */

#ifndef SRC_FB_CO_SESSION_H_
#define SRC_FB_CO_SESSION_H_

#include "protocol/src/poseidon_proto.h"

namespace poseidon {
namespace feedback {

struct SessData {
    enum {
        MEMCAMPAIGN = (1 << 0),
        ADKEY = (1 << 1),
        ADVERTISTER = (1 << 2),
        ADNUM = (1 << 3),
        CREATIVE = (1 << 4),
        DEAL_ID = (1 << 5),
        DEAL_CAMPAIGN = (1 << 6),
        YESTERDAYCOST = (1 << 7),
    };

    FeedbackRequest fbreq;
    FeedbackResponse fbrsp;

    std::vector<std::string> advertiserVt;   // set of advertiser
    std::vector<std::string> campaignVt;     // set of campaign
    std::vector<std::string> adGroupVt;      // set of advertisement ID
    std::vector<std::string> creativeVt;     // set of creative
    std::vector<std::string> dealVt;         // set of deal
    std::vector<std::string> dealCampaignVt; // set of deal campaign
    std::vector<std::string> lastdayAdvertiserVt;  // request: set of yesterday's

    std::map<std::string, std::string> creativeMap;    // response:
    std::map<std::string, std::string> adGroupMap;     // response:
    std::map<std::string, std::string> campaignMap;    // response:
    std::map<std::string, std::string> advertiserMap;  // response:
    std::map<std::string, std::string> countMap;  // response: countMap contains
                                                  // AdNum, campaignNum, ...,
                                                  // etc.
    std::map<std::string, common::FeedbackInfo_PdbFeedback> dealMap;
    std::map<std::string, common::FeedbackInfo_PdbFeedback> dealCampaignMap;

    std::string host;
    int port;
    std::string last_day; // date string: "%Y%M%d"
};

} // namespace feedback
} // namespace poseidon

#endif /* SRC_FB_CO_SESSION_H_ */
