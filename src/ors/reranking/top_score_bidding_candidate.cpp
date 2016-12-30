/**
**/

//include STD C/C++ head files
#include <math.h>

//include third_party_lib head files
#include "src/ors/reranking/top_score_bidding_candidate.h"
#include "util/log.h"

namespace poseidon
{
namespace ors
{
TopScoreBiddingCandidate::TopScoreBiddingCandidate()
{

}
TopScoreBiddingCandidate::~TopScoreBiddingCandidate()
{

}

bool TopScoreBiddingCandidate:: Init()
{
    m_enable_top_score_bidding_flag = true;
    m_top_score_min_threshold = 0.9f;
    return true;
}

void TopScoreBiddingCandidate::Fini()
{

}

int TopScoreBiddingCandidate::BeginWork(QueryAccessor* query_accessor)
{
    m_enable_top_score_bidding_flag = true;
    m_top_score_min_threshold = 0.9f;
 
    AdxBaseParam* base_param = query_accessor->GetAdxBaseParam(); 
    if (base_param->has_enable_top_score_bidding_flag())
    {
        m_enable_top_score_bidding_flag = base_param->enable_top_score_bidding_flag();
        if (base_param->has_top_score_min_threshold())
        {
            m_top_score_min_threshold = base_param->top_score_min_threshold();
        }
    }

    int top_prob_flag = 0;
    m_enable_top_score_prob_flag = false;
    m_top_score_prob_factor = 0.5f;
    if (query_accessor->GetExpParam(EXP_PARAM_ORS_TOP_PROB_FLAG, &top_prob_flag)
        && top_prob_flag == 1) {
        m_enable_top_score_prob_flag = true;
        query_accessor->GetExpParam(EXP_PARAM_ORS_TOP_PROB_FACTOR, &m_top_score_prob_factor);
    }

    LOG_DEBUG("adx_id=%d, pos_id=%s, enable_top_score_bidding_flag=%d, top_score_min_threshold=%f"
            "enable_top_score_prob_flag=%d, top_score_prob_factor=%f",
            query_accessor->GetAdxId(), query_accessor->GetPosId().c_str(),
            m_enable_top_score_bidding_flag, m_top_score_min_threshold,
            m_enable_top_score_prob_flag, m_top_score_prob_factor);
    return 0;
}

void TopScoreBiddingCandidate::SetupCampaignMaxScore(AccessorProvider* accessor_provider)
{
    m_campaign_max_score.clear();
    m_campaign_sum_score.clear();
    for (int i = 0; i < accessor_provider->GetAdNum(); i++)
    {
        AdAccessor* ad_accessor = accessor_provider->GetAdAccessor(i);
        if (ad_accessor->GetFeatureValueInterger(F_ID_X_AD_FILTER) == 1)
        {
            break;
        }

        if (ad_accessor->GetFeatureValueInterger(F_ID_X_AD_IS_CANDIDATE))
        {
            continue;
        }

        float score = ad_accessor->GetFeatureValueFloat(F_ID_X_AD_RANKING_SCORE);
        std::map<uint32_t, float>::iterator iter = m_campaign_max_score.find(ad_accessor->GetCampaignId());
        if (iter == m_campaign_max_score.end())
        {
            m_campaign_max_score[ad_accessor->GetCampaignId()] = score;
            float score_weight = 1.0f;
            m_campaign_sum_score[ad_accessor->GetCampaignId()] = score_weight;
            LOG_DEBUG("ad_id=%d, campaign_id=%d, campaign_max_score=%f, score_weight=%f", 
                    ad_accessor->GetAdId(),ad_accessor->GetCampaignId(), score, score_weight);
        } else {
            float score_weight = pow(m_top_score_prob_factor, log2f(iter->second/score));
            m_campaign_sum_score[ad_accessor->GetCampaignId()] += score_weight;
            LOG_DEBUG("ad_id=%d, campaign_id=%d, score=%f, campaign_max_score=%f, score_weight=%f",
                    ad_accessor->GetAdId(),ad_accessor->GetCampaignId(), score, iter->second, score_weight);
        }
    }

    LOG_DEBUG("SetupCampaignMaxScore OK! campaign_max_score num = %d", m_campaign_max_score.size());
}



int TopScoreBiddingCandidate::Work(AccessorProvider* accessor_provider)
{
    if (!m_enable_top_score_bidding_flag)
    {
        return 0;
    }

    QueryAccessor* query_accessor = accessor_provider->GetQueryAccessor();
    LOG_DEBUG("adx_id=%d, pos_id=%s, AccessorProvider GetAdNum=%d",
            query_accessor->GetAdxId(), 
            query_accessor->GetPosId().c_str(), 
            accessor_provider->GetAdNum());

    if (accessor_provider->GetAdNum() == 0)
    {
        return 0;
    }
    CandidateAdSet* candidate_ad_set = accessor_provider->GetCandidateAdSet();
    this->SetupCampaignMaxScore(accessor_provider);

    for (int i = 0; i < accessor_provider->GetAdNum(); i++)
    {
        AdAccessor* ad_accessor = accessor_provider->GetAdAccessor(i);
        if (ad_accessor->GetFeatureValueInterger(F_ID_X_AD_FILTER) == 1)
        {
            break;
        }

        if (ad_accessor->GetFeatureValueInterger(F_ID_X_AD_IS_CANDIDATE))
        {
            continue;
        }

        float score = ad_accessor->GetFeatureValueFloat(F_ID_X_AD_RANKING_SCORE);
        float campaign_max_score = m_campaign_max_score[ad_accessor->GetCampaignId()];
        float score_weight = pow(m_top_score_prob_factor, log2f(campaign_max_score/score));
        float campaign_sum_score = m_campaign_sum_score[ad_accessor->GetCampaignId()];
        float norm_score_weight = score_weight/campaign_sum_score;

        if (m_enable_top_score_prob_flag) {
            ad_accessor->SetFeature(F_ID_X_AD_TOPN_WEIGHT, norm_score_weight);
        }

        if (m_enable_top_score_prob_flag || score >= campaign_max_score * m_top_score_min_threshold)
        {
            if (!candidate_ad_set->Insert(ad_accessor))
            {
                LOG_WARN("Attention:candidate_ad_set beyond limit");
                break;
            }
            ad_accessor->SetFeature(F_ID_X_AD_IS_CANDIDATE, 1);
            LOG_DEBUG("ad_id=%d insert into candidate_ad_set,score=%f, score_weight=%f, norm_score_weight=%f,"
                "campaign_max_score=%f, campaign_sum_score=%f, topn_weight=%f, enable_top_score_prob_flag=%d", 
                ad_accessor->GetAdId(), score, score_weight, norm_score_weight, campaign_max_score, 
                campaign_sum_score, ad_accessor->GetFeatureValueFloat(F_ID_X_AD_TOPN_WEIGHT), 
                m_enable_top_score_prob_flag);
        }
    }

    LOG_DEBUG("adx_id=%d, pos_id=%s, candidate_ad_set count=%d", 
            query_accessor->GetAdxId(), query_accessor->GetPosId().c_str(),
            candidate_ad_set->GetInsertCount());

    return 0;
}

int TopScoreBiddingCandidate::EndWork(QueryAccessor* /*query_accessor*/)
{
    return 0;
}


} // namespace ors
} // namespace poseidon

