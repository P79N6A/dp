/**
**/

//include STD C/C++ head files
#include <math.h>

//include third_party_lib head files
#include "src/ors/reranking/budget_bidding_topn.h"
#include "util/log.h"

namespace poseidon
{
namespace ors
{
BudgetBiddingTopn::BudgetBiddingTopn()
{

}
BudgetBiddingTopn::~BudgetBiddingTopn()
{

}

bool BudgetBiddingTopn:: Init()
{
    m_enable_budget_bidding_flag = true;
    return true;
}

void BudgetBiddingTopn::Fini()
{

}

int BudgetBiddingTopn::BeginWork(QueryAccessor* query_accessor)
{
    m_enable_budget_bidding_flag = true;
 
    AdxBaseParam* base_param = query_accessor->GetAdxBaseParam(); 
    if (base_param->has_enable_budget_bidding_flag())
    {
        m_enable_budget_bidding_flag = base_param->enable_budget_bidding_flag();
    }

    LOG_DEBUG("adx_id=%d, pos_id=%s, enable_budget_bidding_flag=%d",
            query_accessor->GetAdxId(), query_accessor->GetPosId().c_str(),
            m_enable_budget_bidding_flag);
    return 0;
}

int BudgetBiddingTopn::Work(AccessorProvider* accessor_provider)
{
    if (!m_enable_budget_bidding_flag)
    {
        return 0;
    }

    CandidateAdSet* candidate_ad_set = accessor_provider->GetCandidateAdSet();
    QueryAccessor* query_accessor = accessor_provider->GetQueryAccessor();

    LOG_DEBUG("adx_id=%d, pos_id=%s, candidate_ad_set count=%d",
            query_accessor->GetAdxId(), query_accessor->GetPosId().c_str(),
            candidate_ad_set->GetInsertCount());

    if (candidate_ad_set->GetInsertCount() == 0)
    {
        return 0;
    }

    // 计算推广计划下的广告数
    std::map<uint32_t, uint32_t> campaign_ad_count;
    for (int i = 0; i < candidate_ad_set->GetInsertCount(); i++)
    {
        AdAccessor* ad_accessor = candidate_ad_set->GetCandidateAdAccessors(i);
        if (campaign_ad_count.find(ad_accessor->GetCampaignId()) == campaign_ad_count.end())
        {
            campaign_ad_count[ad_accessor->GetCampaignId()] = 0;
        }
        campaign_ad_count[ad_accessor->GetCampaignId()] += 1;
    }

    // 计算推广计划剩余的最大预算
    float max_budget_left = 0.01f;
    for (int i = 0; i < candidate_ad_set->GetInsertCount(); i++)
    {
        AdAccessor* ad_accessor = candidate_ad_set->GetCandidateAdAccessors(i);
        if (ad_accessor->GetCampaignDailyBudget() < 0)
        {
            continue;
        }

        float budget_left = ad_accessor->GetCampaignDailyBudget() - ad_accessor->GetCampaginDayCost();
        if (budget_left > max_budget_left)
        {
            max_budget_left = budget_left;
        }
        LOG_DEBUG("ad_id=%d, campaign_id=%d, campaign_budget=%f, campaign_cost=%f, budget_left=%f, max_budget_left=%f",
                ad_accessor->GetAdId(), ad_accessor->GetCampaignId(), 
                ad_accessor->GetCampaignDailyBudget(), ad_accessor->GetCampaginDayCost(), 
                budget_left, max_budget_left);
    }

    // 计算预算因子对于广告进行TopN的权重
    for (int i = 0; i < candidate_ad_set->GetInsertCount(); i++)
    {
        AdAccessor* ad_accessor = candidate_ad_set->GetCandidateAdAccessors(i);
        float budget_left = ad_accessor->GetCampaignDailyBudget() - ad_accessor->GetCampaginDayCost();
        if (ad_accessor->GetCampaignDailyBudget() < 0)
        {
            budget_left = max_budget_left;
        }

        // 归一化以及除以广告个数
        float budget_weight = (budget_left/max_budget_left)/campaign_ad_count[ad_accessor->GetCampaignId()];
        float topn_weight = ad_accessor->GetFeatureValueFloat(F_ID_X_AD_TOPN_WEIGHT) * budget_weight;
        ad_accessor->SetFeature(F_ID_X_AD_TOPN_WEIGHT, topn_weight);
        LOG_DEBUG("ad_id=%d, campaign_id=%d, budget_left=%f, max_budget_left=%f, campaign_ad_count=%d, budget_weight=%f, topn_weight=%f", 
                ad_accessor->GetAdId(), ad_accessor->GetCampaignId(),
                budget_left, max_budget_left, campaign_ad_count[ad_accessor->GetCampaignId()],
                budget_weight, topn_weight);
    }

    return 0;
}

int BudgetBiddingTopn::EndWork(QueryAccessor* /*query_accessor*/)
{
    return 0;
}


} // namespace ors
} // namespace poseidon

