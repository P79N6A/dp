/**
**/

//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/scoring/budget_pacing_functor.h"
#include "util/log.h"

namespace poseidon
{
namespace ors
{

BudgetPacingFunctor::BudgetPacingFunctor()
{

}

BudgetPacingFunctor::~BudgetPacingFunctor()
{

}

bool BudgetPacingFunctor::Init()
{
    m_enable_budget_pacing_flag = true;
    return true;
}

void BudgetPacingFunctor::Fini()
{

}

int BudgetPacingFunctor::BeginWork(QueryAccessor* query_accessor)
{
    AdxBaseParam* base_param = query_accessor->GetAdxBaseParam(); 
    m_enable_budget_pacing_flag = true;

    if (base_param->has_enable_budget_pacing_flag())
    {
        m_enable_budget_pacing_flag = base_param->enable_budget_pacing_flag();
    }

    LOG_DEBUG("BudgetPacingFunctor BeginWork OK!enable_budget_pacing_flag=%d", m_enable_budget_pacing_flag);
    return 0;
}

int BudgetPacingFunctor::Work(AdAccessor* ad_accessor, QueryAccessor* /*query_accessor*/)
{
    if (!m_enable_budget_pacing_flag)
    {
        return 0;
    }

    // defalut 0.001 
    // to do
    float ranking_rate_budget_factor = 0.001f;
    if (!ad_accessor->GetFeatureValue(F_ID_X_BUDGET_PACING_RATIO).IsNull())
    {
       ranking_rate_budget_factor = ad_accessor->GetFeatureValueFloat(F_ID_X_BUDGET_PACING_RATIO); 
    }
    ad_accessor->SetFeature(F_ID_X_AD_RANKING_RATE_BUDGET_FACTOR, ranking_rate_budget_factor);
    float ranking_rate = ad_accessor->GetFeatureValueFloat(F_ID_X_AD_RANKING_RATE) * ranking_rate_budget_factor;

    ad_accessor->SetFeature(F_ID_X_AD_RANKING_RATE, ranking_rate);
    LOG_DEBUG("ad_id=%u, ranking_rate_budget_factor=%f, ranking_rate=%f",  
            ad_accessor->GetAdId(), ranking_rate_budget_factor, ranking_rate);
    return 0;
}

int BudgetPacingFunctor::EndWork(QueryAccessor* /*query_accessor*/)
{
    return 0;
}

} // namespace ors
} // namespace poseidon

