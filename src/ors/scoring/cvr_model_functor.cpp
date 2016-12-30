/**
**/

//include STD C/C++ head files

//include third_party_lib head files
#include "src/ors/scoring/cvr_model_functor.h"
#include "util/log.h"
#include "src/model_updater/api/lr_model_predictor.h"

namespace poseidon
{
namespace ors
{

CvrModelFunctor::CvrModelFunctor()
{
    m_default_cvr = 0.01f;
    m_min_cvr = 0.00001f;
}

CvrModelFunctor::~CvrModelFunctor()
{

}

bool CvrModelFunctor::Init()
{
    return true;
}

void CvrModelFunctor::Fini()
{

}

int CvrModelFunctor::BeginWork(QueryAccessor* query_accessor)
{
    m_enable_cvr_promote_flag = true;

    if (query_accessor->GetAdxBaseParam()->has_enable_cvr_factor_flag())
    {
        m_enable_cvr_promote_flag = query_accessor->GetAdxBaseParam()->enable_cvr_factor_flag();
    }

    int cvr_model_id = 0;
    m_enable_cvr_lr_model_flag = false;
    if (query_accessor->GetExpParam(EXP_PARAM_ORS_LR_CVR_MODEL_ID, &cvr_model_id) && cvr_model_id != 0)
    {
         if (!LR_MODEL_SETUP(cvr_model_id))
         {
             LOG_WARN("LR_MODEL_SETUP Failed, cvr_model_id=%d!", cvr_model_id);
         }
         else
         {
             m_enable_cvr_lr_model_flag = true;
             SetLRQueryFea(query_accessor);
         }
    }

    LOG_DEBUG("CvrModelFunctor BeginWork OK!enable_cvr_promote_flag=%d, enable_cvr_lr_model_flag=%d, cvr_model_id=%d", 
        m_enable_cvr_promote_flag, m_enable_cvr_lr_model_flag, cvr_model_id);
    
    return 0;
}

void CvrModelFunctor::SetLRQueryFea(QueryAccessor* query_accessor)
{
    LR_MODEL_FEA_VAL("pid", query_accessor->GetPosId());
    LR_MODEL_FEA_VAL("cnl", query_accessor->GetVideoInfo()->GetFChannelStr());
    LR_MODEL_FEA_VAL("cnl2", query_accessor->GetVideoInfo()->GetSChannelsStr());
    LR_MODEL_FEA_VAL("show_id", query_accessor->GetVideoInfo()->GetShowIdStr());
    LR_MODEL_FEA_VAL("vid", query_accessor->GetVideoInfo()->GetVIdStr());
    LR_MODEL_FEA_VAL("keywords", query_accessor->GetVideoInfo()->GetKeywordsStr());
}

float CvrModelFunctor::GetLRModelCtr(AdAccessor* ad_accessor)
{
    LR_MODEL_FEA_VAL("campaign_id", ad_accessor->GetCampaignId());
    LR_MODEL_FEA_VAL("creative_id", ad_accessor->GetCreativeId());

    return LR_MODEL_SCORE();
}


float CvrModelFunctor::GetStatRateModelCvr(AdAccessor* ad_accessor)
{
    if (!ad_accessor->GetFeatureValue(F_ID_X_ADX_POS_CREATIVE_CVR).IsNull())
    {
        return ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_CREATIVE_CVR);
    }

    if (!ad_accessor->GetFeatureValue(F_ID_X_ADX_POS_AD_CVR).IsNull())
    {
        return  ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_AD_CVR);
    }

    if (!ad_accessor->GetFeatureValue(F_ID_X_ADX_POS_CAMPAIGN_CVR).IsNull())
    {
        return ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_CAMPAIGN_CVR);
    }

    if (!ad_accessor->GetFeatureValue(F_ID_X_ADX_POS_VIEWTYPE_CVR).IsNull())
    {
        return ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_VIEWTYPE_CVR);
    }

    if (!ad_accessor->GetFeatureValue(F_ID_X_ADX_POS_CVR).IsNull())
    {
        return ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_CVR);
    }

    if (!ad_accessor->GetFeatureValue(F_ID_X_ADX_OS_CVR).IsNull())
    {
        return ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_OS_CVR);
    }

   if (!ad_accessor->GetFeatureValue(F_ID_X_ADX_CVR).IsNull())
    {
        return ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_CVR);
    }

    return m_default_cvr;
}

int CvrModelFunctor::Work(AdAccessor* ad_accessor, QueryAccessor* /*query_accessor*/)
{
    float cvr = this->GetStatRateModelCvr(ad_accessor);
    if (m_enable_cvr_lr_model_flag)
    {
        cvr = this->GetLRModelCtr(ad_accessor);
    }
    ad_accessor->SetFeature(F_ID_X_AD_CVR, cvr);

    float ranking_score = ad_accessor->GetFeatureValueFloat(F_ID_X_AD_RANKING_SCORE);
    float ranking_score_cvr_factor = 100 * ad_accessor->GetFeatureValueFloat(F_ID_X_AD_CVR);
    if (m_enable_cvr_promote_flag) 
    {
        ranking_score = ranking_score * ranking_score_cvr_factor;
        ad_accessor->SetFeature(F_ID_X_AD_RANKING_SCORE, ranking_score);
    }
    LOG_DEBUG("ad_id=%d, cvr=%f, ranking_score_cvr_factor=%f, ranking_score=%f", 
            ad_accessor->GetAdId(), cvr, ranking_score_cvr_factor, ranking_score);
    return 0;
}

int CvrModelFunctor::EndWork(QueryAccessor* /*query_accessor*/)
{
    return 0;
}



} // namespace ors
} // namespace poseidon


