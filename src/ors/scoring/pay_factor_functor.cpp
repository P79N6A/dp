/**
**/

//include STD C/C++ head files
#include <algorithm>
#include <math.h>

//include third_party_lib head files
#include "src/ors/scoring/pay_factor_functor.h"
#include "util/log.h"

namespace poseidon
{
namespace ors
{

PayFactorFunctor::PayFactorFunctor()
{

}

PayFactorFunctor::~PayFactorFunctor()
{

}

bool PayFactorFunctor::Init()
{
    return true;
}

void PayFactorFunctor::Fini()
{

}

int PayFactorFunctor::BeginWork(QueryAccessor* query_accessor)
{
    m_enable_pay_factor_flag = false;

    int exp_pay_factor_flag = 0;
    if (query_accessor->GetExpParam(EXP_PRARM_ORS_PAY_FACTOR_FLAG, &exp_pay_factor_flag) 
        && exp_pay_factor_flag == 1) {
            m_enable_pay_factor_flag = true;
    }

    LOG_DEBUG("PayFactorFunctor BeginWork OK!enable_pay_factor_flag=%d, exp_pay_factor_flag=%d", 
            m_enable_pay_factor_flag, exp_pay_factor_flag);
    return 0;
}


int PayFactorFunctor::Work(AdAccessor* ad_accessor, QueryAccessor* /*query_accessor*/)
{
    float ranking_score_pay_factor = 1.0f;
    float ranking_score = ad_accessor->GetFeatureValueFloat(F_ID_X_AD_RANKING_SCORE);
    if (m_enable_pay_factor_flag) {
        if (!ad_accessor->GetFeatureValue(F_ID_X_AD_PAY_FACTOR).IsNull()) {
            ranking_score_pay_factor = ad_accessor->GetFeatureValueFloat(F_ID_X_AD_PAY_FACTOR);
        } else if (!ad_accessor->GetFeatureValue(F_ID_X_CAMPAIGN_PAY_FACTOR).IsNull()) {
            ranking_score_pay_factor = ad_accessor->GetFeatureValueFloat(F_ID_X_CAMPAIGN_PAY_FACTOR);
        }

        ranking_score = ranking_score * ranking_score_pay_factor;
        ad_accessor->SetFeature(F_ID_X_AD_RANKING_SCORE, ranking_score);
    }

    LOG_DEBUG("ad_id=%d, ranking_score_pay_factor=%f, ranking_score=%f",
               ad_accessor->GetAdId(), ranking_score_pay_factor, ranking_score);

    return 0;
}

int PayFactorFunctor::EndWork(QueryAccessor* /*query_accessor*/)
{
    return 0;
}

} // namespace ors
} // namespace poseidon


