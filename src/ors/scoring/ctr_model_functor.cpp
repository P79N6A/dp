/**
**/

//include STD C/C++ head files
#include <algorithm>
#include <math.h>

//include third_party_lib head files
#include "src/ors/scoring/ctr_model_functor.h"
#include "util/log.h"
#include "src/model_updater/api/lr_model_predictor.h"

namespace poseidon
{
namespace ors
{

CtrModelFunctor::CtrModelFunctor()
{
    m_default_ctr = 0.01f;
    m_min_ctr = 0.0001f;
}

CtrModelFunctor::~CtrModelFunctor()
{

}

bool CtrModelFunctor::Init()
{
    return true;
}

void CtrModelFunctor::Fini()
{

}

int CtrModelFunctor::BeginWork(QueryAccessor* query_accessor)
{
    m_enable_ctr_promote_flag = true;
    if (query_accessor->GetAdxBaseParam()->has_enable_ctr_factor_flag())
    {
       m_enable_ctr_promote_flag = query_accessor->GetAdxBaseParam()->enable_ctr_factor_flag();
    }

    int ctr_model_id = 0;
    m_enable_ctr_lr_model_flag = false;
    if (query_accessor->GetExpParam(EXP_PARAM_ORS_LR_CTR_MODEL_ID, &ctr_model_id) && ctr_model_id != 0)
    {
        if (!LR_MODEL_SETUP(ctr_model_id))
        {
            LOG_WARN("LR_MODEL_SETUP Failed, ctr_model_id=%d!", ctr_model_id);
        }
        else
        {
            m_enable_ctr_lr_model_flag = true;
            SetLRQueryFea(query_accessor);
        }
    }

    LOG_DEBUG("CtrModelFunctor BeginWork OK!enable_ctr_promote_flag=%d, enable_ctr_lr_model_flag=%d, ctr_model_id=%d", 
            m_enable_ctr_promote_flag, m_enable_ctr_lr_model_flag, ctr_model_id);
    return 0;
}

void CtrModelFunctor::SetLRQueryFea(QueryAccessor* query_accessor)
{
    LR_MODEL_FEA_VAL("pid", query_accessor->GetPosId());
    LR_MODEL_FEA_VAL("cnl", query_accessor->GetVideoInfo()->GetFChannelStr());
    LR_MODEL_FEA_VAL("cnl2", query_accessor->GetVideoInfo()->GetSChannelsStr());
    LR_MODEL_FEA_VAL("show_id", query_accessor->GetVideoInfo()->GetShowIdStr());
    LR_MODEL_FEA_VAL("vid", query_accessor->GetVideoInfo()->GetVIdStr());
    LR_MODEL_FEA_VAL("keywords", query_accessor->GetVideoInfo()->GetKeywordsStr());
}

float CtrModelFunctor::GetLRModelCtr(AdAccessor* ad_accessor)
{
    LR_MODEL_FEA_VAL("campaign_id", ad_accessor->GetCampaignId());
    LR_MODEL_FEA_VAL("creative_id", ad_accessor->GetCreativeId());

    return LR_MODEL_SCORE();
}

float CtrModelFunctor::GetStatRateModelCtr(AdAccessor* ad_accessor)
{
    if (!ad_accessor->GetFeatureValue(F_ID_X_ADX_POS_CREATIVE_CTR).IsNull())
    {
        return ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_CREATIVE_CTR);
    }

    if (!ad_accessor->GetFeatureValue(F_ID_X_ADX_POS_AD_CTR).IsNull())
    {
        return  ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_AD_CTR);
    }

    if (!ad_accessor->GetFeatureValue(F_ID_X_ADX_POS_CAMPAIGN_CTR).IsNull())
    {
        return ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_CAMPAIGN_CTR);
    }

    if (!ad_accessor->GetFeatureValue(F_ID_X_ADX_POS_VIEWTYPE_CTR).IsNull())
    {
        return ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_VIEWTYPE_CTR);
    }

    if (!ad_accessor->GetFeatureValue(F_ID_X_ADX_POS_CTR).IsNull())
    {
        return ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_CTR);
    }

    if (!ad_accessor->GetFeatureValue(F_ID_X_ADX_OS_CTR).IsNull())
    {
        return ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_OS_CTR);
    }

    if (!ad_accessor->GetFeatureValue(F_ID_X_ADX_CTR).IsNull())
    {
        return ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_CTR);
    }

    return m_default_ctr;
}

int CtrModelFunctor::Work(AdAccessor* ad_accessor, QueryAccessor* /*query_accessor*/)
{
    float ctr = this->GetStatRateModelCtr(ad_accessor);
    if (m_enable_ctr_lr_model_flag)
    {
        ctr  = this->GetLRModelCtr(ad_accessor);
    }
    ad_accessor->SetFeature(F_ID_X_AD_CTR, ctr);

    float ranking_score = ad_accessor->GetFeatureValueFloat(F_ID_X_AD_RANKING_SCORE);
    float ranking_score_ctr_factor = 100 * ad_accessor->GetFeatureValueFloat(F_ID_X_AD_CTR);
    if (m_enable_ctr_promote_flag)
    {
        ranking_score = ranking_score * ranking_score_ctr_factor;
        ad_accessor->SetFeature(F_ID_X_AD_RANKING_SCORE, ranking_score);
    }

    LOG_DEBUG("ad_id=%d, ctr=%f, ranking_score_ctr_factor=%f, ranking_score=%f", 
            ad_accessor->GetAdId(), ctr, ranking_score_ctr_factor, ranking_score);
    return 0;
}

int CtrModelFunctor::EndWork(QueryAccessor* /*query_accessor*/)
{
    return 0;
}



} // namespace ors
} // namespace poseidon


