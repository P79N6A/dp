/**
**/

//include STD C/C++ head files
#include <string.h>

//include third_party_lib head files
#include "src/ors/common/ad_accessor.h"
#include "util/log.h"
#include "src/ors/common/pro_stat.h"

namespace poseidon
{
namespace ors
{

AdAccessor::AdAccessor()
{

}

AdAccessor::~AdAccessor()
{

}

int AdAccessor::Bind(const AlgoAd& algo_ad) 
{
    for (int i = 0; i < F_ID_MAX_NUM; i++)
    {
        m_features[i].SetNull(); 
    }

    this->SetFeature(F_ID_X_AD_RANKING_SCORE, 1.0f);
    this->SetFeature(F_ID_X_AD_RANKING_RATE, 1.0f);
    this->SetFeature(F_ID_X_AD_FILTER, 0);
    this->SetFeature(F_ID_X_AD_BIDDING_EXPLORE_FLAG, 0);
    this->SetFeature(F_ID_X_BIDDING_MODE, 0);
    this->SetFeature(F_ID_X_AD_TOPN_WEIGHT, 1.0f);
    this->SetFeature(F_ID_X_AD_IS_CANDIDATE, 0);
    this->SetFeature(F_ID_X_AD_IS_TOPN, 0);

    m_index_id = algo_ad.id();
    m_campaign_id = algo_ad.ad().campaign_id();
    m_creative_id = algo_ad.ad().creative_id();
    m_ad_id = algo_ad.ad().adgroup_id();
    m_advertiser_id = algo_ad.ad().advertiser_id();
    m_org_price = algo_ad.ad().org_price();
    this->SetFeature(F_ID_X_ORG_PRICE, m_org_price/100.0f);
    m_billing_type = atoi(algo_ad.ad().billing_type().c_str());
    m_view_type = algo_ad.ad().view_type();
    m_premium_rate = algo_ad.ad().premium_rate() / 100.0f;
    m_campaign_daily_budget = -1.0f;
    if (algo_ad.ad().has_campaign_daily_budget())
    {
        m_campaign_daily_budget = algo_ad.ad().campaign_daily_budget() / 100.0f;
    }
    m_ad_user_day_freq = 0;
    m_campaign_user_day_freq = 0;
    m_campaign_day_cost = 0;

    PRO_STAT(TPROPERTY_ID_ADVERTISER_REQ, m_advertiser_id, 1);
    PRO_STAT(TPROPERTY_ID_CAMPAIGN_REQ, m_campaign_id, 1);
    PRO_STAT(TPROPERTY_ID_AD_REQ, m_ad_id, 1);
    PRO_STAT(TPROPERTY_ID_CREATIVE_REQ, m_creative_id, 1);
    LOG_DEBUG("Bind ad_id = %d, campaign_id=%d, advertiser_id=%d, org_price=%d, billing_type=%d"
            "premium_rate=%f, campaign_daily_budget=%f",
            m_ad_id, m_campaign_id, m_advertiser_id, m_org_price, m_billing_type,
            m_premium_rate, m_campaign_daily_budget);
    return 0;
}

int AdAccessor::Bind(const common::FeedbackInfo& fb_info)
{
    m_ad_user_day_freq = fb_info.adgroup_user_day_freq();
    m_campaign_user_day_freq = fb_info.campaign_user_day_freq();
    m_campaign_day_cost = fb_info.campaign_day_cost() / 100.0f;

    LOG_DEBUG("Bind ad_id=%d, ad_user_day_freq=%d, campaign_user_day_freq=%d, campaign_day_cost=%f", 
            m_ad_id, m_ad_user_day_freq, m_campaign_user_day_freq, m_campaign_day_cost);

    return 0;
}


} // namespace ors
} // namespace poseidon

