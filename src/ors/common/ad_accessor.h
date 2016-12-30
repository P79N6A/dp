/**
**/

#ifndef _ORS_COMMON_AD_ACCESSOR_H_
#define _ORS_COMMON_AD_ACCESSOR_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "protocol/src/poseidon_proto.h"
#include "src/ors/common/common_def.h"

namespace poseidon
{
namespace ors
{
class AdAccessor
{

public:
    AdAccessor();
    virtual ~AdAccessor();
    virtual int Bind(const AlgoAd& algo_ad);
    virtual int Bind(const common::FeedbackInfo& fb_info);

    uint32_t GetIndexId()
    {
        return m_index_id;
    }

    uint32_t GetCreativeId()
    {
        return m_creative_id;
    }

    uint32_t GetAdId() 
    {
        return m_ad_id;
    }

    uint32_t GetCampaignId()
    {
        return m_campaign_id;
    }

    uint32_t GetAdvertiserId()
    {
        return m_advertiser_id;
    }

    uint32_t GetOrgPrice()
    {
        return m_org_price;
    }

    uint32_t GetBillingType() 
    {
        return m_billing_type;
    }

    int GetAdUserFreq()
    {
        return m_ad_user_day_freq;
    }

    int GetCampaignUserFreq()
    {
        return m_campaign_user_day_freq;
    }

    float GetCampaignDailyBudget()
    {
        return m_campaign_daily_budget;
    }

    float GetCampaginDayCost()
    {
        return m_campaign_day_cost;
    }

    float GetPremiumRate()
    {
        return m_premium_rate;
    }

    uint32_t GetViewType()
    {
        return m_view_type;
    }

    void SetFeature(int fid, int fval)
    {
        m_features[fid].u.fixed = fval;
    }

    void SetFeature(int fid, float fval)
    {
        m_features[fid].u.real = fval;
    }

    int GetFeatureValueInterger(int fid)
    {
        return m_features[fid].u.fixed;
    }

    float GetFeatureValueFloat(int fid)
    {
        return m_features[fid].u.real;
    }

    FValue GetFeatureValue(int fid)
    {
        return m_features[fid];
    }


protected:
    uint32_t m_index_id;
    uint32_t m_creative_id;
    uint32_t m_ad_id;
    uint32_t m_campaign_id;
    uint32_t m_advertiser_id;
    uint32_t m_org_price;
    uint32_t m_billing_type;
    uint32_t m_view_type;
  
    int m_ad_user_day_freq;
    int m_campaign_user_day_freq;
    float m_campaign_day_cost;
    float m_campaign_daily_budget;

    float m_premium_rate;

    FValue m_features[F_ID_MAX_NUM];
};


} // namespace ors
} // namespace poseidon

#endif // _ORS_COMMON_AD_ACCESSOR_H_

