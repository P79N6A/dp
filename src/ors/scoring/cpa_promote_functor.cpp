/**
**/

//include STD C/C++ head files
#include <algorithm>
#include <math.h>

//include third_party_lib head files
#include "src/ors/scoring/cpa_promote_functor.h"
#include "util/log.h"

namespace poseidon
{
namespace ors
{

CpaPromoteFunctor::CpaPromoteFunctor()
{

}

CpaPromoteFunctor::~CpaPromoteFunctor()
{

}

bool CpaPromoteFunctor::Init()
{
    return true;
}

void CpaPromoteFunctor::Fini()
{

}

int CpaPromoteFunctor::BeginWork(QueryAccessor* query_accessor)
{
    m_enable_cpa_promote_flag = false;
    m_cpa_promote_factor = 0.0f;

    if (query_accessor->GetAdxBaseParam()->has_enable_cpa_promote_flag())
    {
        m_enable_cpa_promote_flag = query_accessor->GetAdxBaseParam()->enable_cpa_promote_flag();
        m_cpa_promote_factor = query_accessor->GetAdxBaseParam()->cpa_promote_factor();
    }

    LOG_DEBUG("CpaPromoteFunctor BeginWork OK!enable_cpa_promote_flag=%d, cpa_promote_factor=%f", 
            m_enable_cpa_promote_flag, m_cpa_promote_factor);
    return 0;
}


int CpaPromoteFunctor::Work(AdAccessor* ad_accessor, QueryAccessor* /*query_accessor*/)
{
    float ranking_rate_cpa_factor = 1.0f;
    if (m_enable_cpa_promote_flag &&
        !ad_accessor->GetFeatureValue(F_ID_X_ADX_CPA).IsNull() &&
        !ad_accessor->GetFeatureValue(F_ID_X_ADX_POS_CREATIVE_CPA).IsNull()) {
        float base_cpa = ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_CPA) * 2;
        float ad_cpa = ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_CREATIVE_CPA);
        if (ad_cpa > FZERO && base_cpa > FZERO) {
            float ratio = std::min(3.0f, base_cpa/ad_cpa);
            ranking_rate_cpa_factor = pow(ratio, m_cpa_promote_factor);
        }
        LOG_DEBUG("ad_id=%d, base_cpa=%f, ad_cpa=%f, ranking_rate_cpa_factor=%f", 
                ad_accessor->GetAdId(), base_cpa, ad_cpa, ranking_rate_cpa_factor);
    }

    ad_accessor->SetFeature(F_ID_X_AD_RANKING_RATE_CPA_FACTOR, ranking_rate_cpa_factor);  
    float ranking_rate = ad_accessor->GetFeatureValueFloat(F_ID_X_AD_RANKING_RATE) * ranking_rate_cpa_factor;
    ad_accessor->SetFeature(F_ID_X_AD_RANKING_RATE, ranking_rate);

    LOG_DEBUG("ad_id=%d, ranking_rate_cpa_factor=%f, ranking_rate=%f",
               ad_accessor->GetAdId(), ranking_rate_cpa_factor, ranking_rate);

    return 0;
}

int CpaPromoteFunctor::EndWork(QueryAccessor* /*query_accessor*/)
{
    return 0;
}

} // namespace ors
} // namespace poseidon


