/**
**/

#ifndef _ORS_TOP_SCORE_BIDDING_CANDIDATE_H_
#define _ORS_TOP_SCORE_BIDDING_CANDIDATE_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/functor.h"

namespace poseidon
{
namespace ors
{
class TopScoreBiddingCandidate : public Functor
{

public:
    TopScoreBiddingCandidate();
    virtual ~TopScoreBiddingCandidate();
    virtual bool Init();
    virtual void Fini();

    virtual int BeginWork(QueryAccessor* query_accessor);
    virtual int Work(AccessorProvider* accessor_provider);
    virtual int EndWork(QueryAccessor* query_accessor);

protected:
    void SetupCampaignMaxScore(AccessorProvider* accessor_provider);
    void SetupScoreTopnWeight(AccessorProvider* accessor_provider);
protected:    
    bool m_enable_top_score_bidding_flag;
    float m_top_score_min_threshold;
    std::map<uint32_t, float> m_campaign_max_score;
    std::map<uint32_t, float> m_campaign_sum_score;
    
    bool m_enable_top_score_prob_flag;
    float m_top_score_prob_factor;
};
} // namespace ors
} // namespace poseidon

#endif // _ORS_TOP_SCORE_BIDDING_CANDIDATE_H_

