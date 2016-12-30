/**
**/

//include STD C/C++ head files
#include <math.h>

//include third_party_lib head files
#include "src/ors/scoring/frequency_smooth_functor.h"
#include "util/log.h"

namespace poseidon
{
namespace ors
{

FrequencySmoothFunctor::FrequencySmoothFunctor()
{
}

FrequencySmoothFunctor::~FrequencySmoothFunctor()
{

}

bool FrequencySmoothFunctor::Init()
{
    m_enable_frequency_smooth_flag = true;
    m_frequency_smooth_factor = 1.0f;
    return true;
}

void FrequencySmoothFunctor::Fini()
{

}

int FrequencySmoothFunctor::BeginWork(QueryAccessor* query_accessor)
{
    AdxBaseParam* base_param = query_accessor->GetAdxBaseParam(); 
    m_enable_frequency_smooth_flag = true;
    m_frequency_smooth_factor = 1.0f;
    m_creative_frequency_limit_cnt = 0;
    m_campaign_frequency_limit_cnt = 0;

    if (base_param->has_enable_frequency_smooth_flag())
    {
        m_enable_frequency_smooth_flag = base_param->enable_frequency_smooth_flag();
        if (base_param->has_frequency_smooth_factor())
        {
            m_frequency_smooth_factor = base_param->frequency_smooth_factor();
        }
    }

    query_accessor->GetExpParam(EXP_PARAM_ORS_CREATIVE_FREQUENCY_LIMIT_CNT, &m_creative_frequency_limit_cnt);
    query_accessor->GetExpParam(EXP_PARAM_ORS_CAMPAIGN_FREQUENCY_LIMIT_CNT, &m_campaign_frequency_limit_cnt);

    LOG_DEBUG("enable_frequency_smooth_flag=%d, frequency_smooth_factor=%f," 
            "creative_frequency_limit_cnt=%d, campaign_frequency_limit_cnt=%d", 
            m_enable_frequency_smooth_flag, m_frequency_smooth_factor,
            m_creative_frequency_limit_cnt, m_campaign_frequency_limit_cnt);
    return 0;
}

int FrequencySmoothFunctor::Work(AdAccessor* ad_accessor, QueryAccessor* /*query_accessor*/)
{
    if (!m_enable_frequency_smooth_flag)
    {
        return 0;
    }

    // only ad frequency smooth support 
    // to do more rules with campaign/advertiser
   
    int ad_user_day_freq = ad_accessor->GetAdUserFreq();
    int campaign_user_day_freq = ad_accessor->GetCampaignUserFreq();

    float ranking_score_frequency_factor = 1.0f;
    if (ad_user_day_freq > 0)
    {
        ranking_score_frequency_factor = 1 / pow(m_frequency_smooth_factor, ad_user_day_freq);  
    }

    ad_accessor->SetFeature(F_ID_X_AD_RANKING_SCORE_FREQUENCY_FACTOR, ranking_score_frequency_factor);
    float ranking_score = ad_accessor->GetFeatureValueFloat(F_ID_X_AD_RANKING_SCORE) * ranking_score_frequency_factor;
    ad_accessor->SetFeature(F_ID_X_AD_RANKING_SCORE, ranking_score);

    float ranking_rate_frequency_factor = 1.0f;
    if (m_creative_frequency_limit_cnt > 0 && ad_user_day_freq >= m_creative_frequency_limit_cnt) {
        ranking_rate_frequency_factor = FZERO; 
    }

    if (m_campaign_frequency_limit_cnt > 0 && campaign_user_day_freq >= m_campaign_frequency_limit_cnt) {
        ranking_rate_frequency_factor = FZERO;
    }

    float ranking_rate = ad_accessor->GetFeatureValueFloat(F_ID_X_AD_RANKING_RATE) * ranking_rate_frequency_factor;
    ad_accessor->SetFeature(F_ID_X_AD_RANKING_RATE, ranking_rate);

    LOG_DEBUG("ad_id=%d, ad_user_day_freq=%d, campaign_user_day_freq=%d, ranking_score_frequency_factor=%f, ranking_score=%f,"
            "ranking_rate_frequency_factor=%f, ranking_rate=%f",
            ad_accessor->GetAdId(), ad_user_day_freq, ranking_score_frequency_factor, ranking_score,
            ranking_rate_frequency_factor, ranking_rate);
    return 0;
}

int FrequencySmoothFunctor::EndWork(QueryAccessor* /*query_accessor*/)
{
    return 0;
}

} // namespace ors
} // namespace poseidon

