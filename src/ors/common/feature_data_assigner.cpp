/**
**/

//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/feature_data_assigner.h"
#include "third_party/cityhash/include/city.h"
#include "util/log.h"
#include "util/func.h"
#include "src/ors/common/config_center.h"
#include "util/date_time.h"
#include "src/model_updater/api/algo_model_data_api.h"

using namespace poseidon::model_updater;

namespace poseidon
{
namespace ors
{

FeatureDataAssigner::FeatureDataAssigner()
{

}

FeatureDataAssigner::~FeatureDataAssigner()
{

}

bool FeatureDataAssigner::Init()
{
    LOG_INFO("FeatureDataAssigner Init OK!");
    return true;
}

void FeatureDataAssigner::Fini()
{
    AlgoModelDataApi::get_mutable_instance().Fini();
}

bool FeatureDataAssigner::Bind(QueryAccessor* query_accessor)
{
    m_adx_id = query_accessor->GetAdxId();
    m_os_type = query_accessor->GetOsType();
    m_pos_id = query_accessor->GetPosHashId();
    m_view_type = query_accessor->GetViewType();

    this->AssignBaseParam(query_accessor);
    this->AssignSpotGradeFeature(query_accessor);
    this->AssignVideoContextGradeFeature(query_accessor);

    LOG_DEBUG("FeatureDataAssigner Bind QueryAccessor OK!");
    return true;
}

void FeatureDataAssigner::AssignBaseParam(QueryAccessor* query_accessor)
{
    if (AlgoModelDataApi::get_mutable_instance().GetBaseParamValue(m_adx_id, query_accessor->GetAdxBaseParam()))
    {
        LOG_DEBUG("GetBaseParamValue adx_id=%d OK", m_adx_id);
    }
    if (AlgoModelDataApi::get_mutable_instance().GetBaseParamValue(m_adx_id, m_pos_id, query_accessor->GetPosBaseParam()))
    {
        LOG_DEBUG("GetBaseParamValue adx_id=%d, pos_id=%lu OK", m_adx_id, m_pos_id);
    }
}

void FeatureDataAssigner::AssignSpotGradeFeature(QueryAccessor* query_accessor)
{
    SpotGradeKey key;
    SpotGradeValue* value = NULL;

    key.source = m_adx_id;
    key.pid = m_pos_id;
    key.app_id = query_accessor->GetAppHashId();

    if (AlgoModelDataApi::get_mutable_instance().GetSpotGradeValue(key, &value))
    {
        query_accessor->SetTrace(T_ID_CONTEXT_SPOT_GRADE_QUALITY, value->quality);
        LOG_DEBUG("GetSpotGradeValue adx_id=%d, pos_id=%lu, app_id=%lu, quality=%f", 
                key.source, key.pid, key.app_id, value->quality);
        return;
    }
}

void FeatureDataAssigner::AssignVideoContextGradeFeature(QueryAccessor* query_accessor)
{
    if (!query_accessor->IsVideo())
    {
        return;
    }

    VideoContextGradeKey key;
    VideoContextGradeValue* value = NULL;
    VideoInfo* video_info = query_accessor->GetVideoInfo();

    key.source = query_accessor->GetAdxId();
    key.fchannel = video_info->GetFChannel();
    
    // 视频ID
    if (video_info->HasVId())
    {
        key.context = video_info->GetVId();
        key.context_type = VIDEO_CONTEXT_TYPE_VID;
        if (AlgoModelDataApi::get_mutable_instance().GetVideoContextGradeValue(key, &value))
        {
            query_accessor->SetTrace(T_ID_CONTEXT_VIDEO_GRADE_QUALITY, value->quality);
            LOG_DEBUG("GetVideoContextGradeValue vid = %lu quality=%f", key.context, value->quality);
            return;
        }
    }

    // 节目ID
    if (video_info->HasShowId())
    {
        key.context = video_info->GetShowId();
        key.context_type = VIDEO_CONTEXT_TYPE_SHOW_ID;
        if (AlgoModelDataApi::get_mutable_instance().GetVideoContextGradeValue(key, &value))
        {
            query_accessor->SetTrace(T_ID_CONTEXT_VIDEO_GRADE_QUALITY, value->quality);
            LOG_DEBUG("GetVideoContextGradeValue show id = %lu quality=%f", key.context, value->quality);
            return;
        }
    }

    // 专辑ID
    // todo
    
    // title
    if (m_adx_id != ADX_ID_YOUKU && video_info->HasTitle())
    {
        float max_quality = -1.0f;
        for (size_t i = 0; i < video_info->GetTitleSegments().size(); i++)
        {
            key.context = video_info->GetTitleSegments()[i]; 
            key.context_type = VIDEO_CONTEXT_TYPE_TITLE;
            if (AlgoModelDataApi::get_mutable_instance().GetVideoContextGradeValue(key, &value))
            {
                max_quality =std::max(max_quality, value->quality);
            }
        }
        if (max_quality >= 0) 
        {   
            query_accessor->SetTrace(T_ID_CONTEXT_VIDEO_GRADE_QUALITY, max_quality);
        
            LOG_DEBUG("GetVideoContextGradeValue title max quality=%f", max_quality);
            return;
        }
    }

    // 关键词
    if (video_info->HasKeyWord())
    {
        float max_quality = -1.0f;
        for (size_t i = 0; i < video_info->GetKeywords().size(); i++)
        {
            key.context = video_info->GetKeywords()[i];
            key.context_type = VIDEO_CONTEXT_TYPE_KEYWORD;
            if (AlgoModelDataApi::get_mutable_instance().GetVideoContextGradeValue(key, &value))
            {
                max_quality =std::max(max_quality, value->quality);
            }
        }
        if (max_quality >= 0) {
            query_accessor->SetTrace(T_ID_CONTEXT_VIDEO_GRADE_QUALITY, max_quality);
            
            LOG_DEBUG("GetVideoContextGradeValue keywords max quality=%f", max_quality);
            return;
        }
    }

    // 二级分类
    if (video_info->hasSChannel())
    {
        float max_quality = -1.0f;
        for (size_t i = 0; i < video_info->GetSChannels().size(); i++)
        {
            key.context = video_info->GetSChannels()[i];
            key.context_type = VIDEO_CONTEXT_TYPE_SCHANNEL;
            if (AlgoModelDataApi::get_mutable_instance().GetVideoContextGradeValue(key, &value))
            {
                max_quality =std::max(max_quality, value->quality);
            }
        }

        if (max_quality > 0) {
            query_accessor->SetTrace(T_ID_CONTEXT_VIDEO_GRADE_QUALITY, max_quality);
        
            LOG_DEBUG("GetVideoContextGradeValue schannel max quality=%f", max_quality);
            return;
        }
    }

    // 一级分类
    if (video_info->HasFChannel())
    {
        key.context_type = VIDEO_CONTEXT_TYPE_FCHANNEL;
        key.context = video_info->GetFChannel();
        if (AlgoModelDataApi::get_mutable_instance().GetVideoContextGradeValue(key, &value))
        {
            query_accessor->SetTrace(T_ID_CONTEXT_VIDEO_GRADE_QUALITY, value->quality);
            LOG_DEBUG("GetVideoContextGradeValue fchannel quality=%f", value->quality);
            return;
        }
    }

    LOG_DEBUG("GetVideoContextGradeValue None!");
    return;
}

bool FeatureDataAssigner::Assign(AdAccessor* ad_accessor)
{
    AssignStatRateFeature(ad_accessor);
    AssinBudgetPacingFeature(ad_accessor);
    AssignPayFactorFeature(ad_accessor);
    LOG_DEBUG("FeatureDataAssigner Assign Ad=%d OK", ad_accessor->GetAdId());
    return true;
}

void FeatureDataAssigner::AssignPayFactorFeature(AdAccessor* ad_accessor)
{
    PayFactorKey key;
    PayFactorValue* value = NULL;

    key.campaign_id = ad_accessor->GetCampaignId();
    if (AlgoModelDataApi::get_mutable_instance().GetPayFactorValue(key, &value))
    {
        ad_accessor->SetFeature(F_ID_X_CAMPAIGN_PAY_FACTOR, value->pay_factor);
    }

    key.ad_id = ad_accessor->GetAdId();
    if (AlgoModelDataApi::get_mutable_instance().GetPayFactorValue(key, &value))
    {
        ad_accessor->SetFeature(F_ID_X_AD_PAY_FACTOR, value->pay_factor);
    }

    LOG_DEBUG("GetPayFactorValue, campaign_id=%u, ad_id=%u,"
            " campaign_pay_factor=%f, ad_pay_factor=%f",
            key.campaign_id, key.ad_id,
            ad_accessor->GetFeatureValueFloat(F_ID_X_CAMPAIGN_PAY_FACTOR),
             ad_accessor->GetFeatureValueFloat(F_ID_X_AD_PAY_FACTOR));
}

void FeatureDataAssigner::AssinBudgetPacingFeature(AdAccessor* ad_accessor)
{
    BudgetPacingKey key;
    BudgetPacingValue* value = NULL;

    key.source = m_adx_id;
    key.campaign_id = ad_accessor->GetCampaignId();
    
    if (AlgoModelDataApi::get_mutable_instance().GetBudgetPacingValue(key, &value))
    {
        ad_accessor->SetFeature(F_ID_X_BUDGET_PACING_RATIO, value->budget_pacing_ratio);
        ad_accessor->SetFeature(F_ID_X_BIDDING_MODE, value->bid_mode);
        ad_accessor->SetFeature(F_ID_X_BIDDING_FIXED_PRICE, value->fixed_price);
    }

    LOG_DEBUG("get budget pacing value campaign_id=%u, ratio=%f",
             ad_accessor->GetCampaignId(), 
             ad_accessor->GetFeatureValueFloat(F_ID_X_BUDGET_PACING_RATIO));
}

void FeatureDataAssigner::AssignStatRateFeature(AdAccessor* ad_accessor)
{
    StatRateKey key;
    StatRateValue* value = NULL;
    
    LOG_DEBUG("get advertiser_id=%d, campagin_id=%d, ad_id=%d creative_id=%d stat rate value", 
            ad_accessor->GetAdvertiserId(), 
            ad_accessor->GetCampaignId(), 
            ad_accessor->GetAdId(),
            ad_accessor->GetCreativeId());

    key.source = m_adx_id;
    if (!AlgoModelDataApi::get_mutable_instance().GetStatRateValue(key, &value))
    {
        return;
    }
    ad_accessor->SetFeature(F_ID_X_ADX_CTR, value->ctr);  
    ad_accessor->SetFeature(F_ID_X_ADX_CVR, value->cvr);
    ad_accessor->SetFeature(F_ID_X_ADX_CPA, value->cpa);
    LOG_DEBUG("adx=%d, ctr=%f, cvr=%f, cpa=%f", m_adx_id,
            ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_CTR), 
            ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_CVR),
            ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_CPA));

    key.os_type = m_os_type;
    if (!AlgoModelDataApi::get_mutable_instance().GetStatRateValue(key, &value))
    {
        return;
    }
    ad_accessor->SetFeature(F_ID_X_ADX_OS_CTR, value->ctr);  
    ad_accessor->SetFeature(F_ID_X_ADX_OS_CVR, value->cvr);
    ad_accessor->SetFeature(F_ID_X_ADX_OS_CPA, value->cpa);
    LOG_DEBUG("os_type=%d, ctr=%f, cvr=%f, cpa=%f", m_os_type,
            ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_OS_CTR), 
            ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_OS_CVR),
            ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_OS_CPA));


    key.pid = m_pos_id;
    if (!AlgoModelDataApi::get_mutable_instance().GetStatRateValue(key, &value))
    {
        return;
    }
    ad_accessor->SetFeature(F_ID_X_ADX_POS_CTR, value->ctr);  
    ad_accessor->SetFeature(F_ID_X_ADX_POS_CVR, value->cvr); 
    LOG_DEBUG("pos=%lu, ctr=%f, cvr=%f", m_pos_id,
            ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_CTR), 
            ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_CVR));

    key.view_type = m_view_type;
    if (ad_accessor->GetViewType() != 0)
    {
        key.view_type = ad_accessor->GetViewType();
    }
    if (!AlgoModelDataApi::get_mutable_instance().GetStatRateValue(key, &value))
    {
        return;
    }
    ad_accessor->SetFeature(F_ID_X_ADX_POS_VIEWTYPE_CTR, value->ctr);  
    ad_accessor->SetFeature(F_ID_X_ADX_POS_VIEWTYPE_CVR, value->cvr); 
    ad_accessor->SetFeature(F_ID_X_ADX_POS_VIEWTYPE_CPA, value->cpa);
    LOG_DEBUG("view_type=%d, ctr=%f, cvr=%f, cpa=%f", key.view_type,
            ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_VIEWTYPE_CTR), 
            ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_VIEWTYPE_CVR),
            ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_VIEWTYPE_CPA));

    key.campaign_id = ad_accessor->GetCampaignId();
    if (!AlgoModelDataApi::get_mutable_instance().GetStatRateValue(key, &value))
    {
        LOG_DEBUG("key=%s not found", key.to_string());
        return;
    }
    ad_accessor->SetFeature(F_ID_X_ADX_POS_CAMPAIGN_CTR, value->ctr);  
    ad_accessor->SetFeature(F_ID_X_ADX_POS_CAMPAIGN_CVR, value->cvr); 
    ad_accessor->SetFeature(F_ID_X_ADX_POS_CAMPAIGN_CPA, value->cpa);
    LOG_DEBUG("campaign_id=%d, ctr=%f, cvr=%f, cpa=%f", key.campaign_id,
            ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_CAMPAIGN_CTR),
            ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_CAMPAIGN_CVR),
            ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_CAMPAIGN_CPA));

    key.ad_id = ad_accessor->GetAdId();
    if (!AlgoModelDataApi::get_mutable_instance().GetStatRateValue(key, &value))
    {
        return;
    }
    ad_accessor->SetFeature(F_ID_X_ADX_POS_AD_CTR, value->ctr);  
    ad_accessor->SetFeature(F_ID_X_ADX_POS_AD_CVR, value->cvr); 
    LOG_DEBUG("ad_id=%d, ctr=%f, cvr=%f", key.ad_id, 
            ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_AD_CTR),
            ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_AD_CVR));

    key.creative_id = ad_accessor->GetCreativeId();
    if (!AlgoModelDataApi::get_mutable_instance().GetStatRateValue(key, &value))
    {
        return;
    }
    ad_accessor->SetFeature(F_ID_X_ADX_POS_CREATIVE_CTR, value->ctr);  
    ad_accessor->SetFeature(F_ID_X_ADX_POS_CREATIVE_CVR, value->cvr); 
    ad_accessor->SetFeature(F_ID_X_ADX_POS_CREATIVE_CPM, value->cpm);
    ad_accessor->SetFeature(F_ID_X_ADX_POS_CREATIVE_CPC, value->cpc);
    //CPA用新增
    ad_accessor->SetFeature(F_ID_X_ADX_POS_CREATIVE_CPA, value->cpa);
    LOG_DEBUG("creative_id=%d, ctr=%f, cvr=%f, cpm=%f, cpc=%f, cpa=%f", key.creative_id,
            ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_CREATIVE_CTR),
            ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_CREATIVE_CVR),
            ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_CREATIVE_CPM),
            ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_CREATIVE_CPC),
            ad_accessor->GetFeatureValueFloat(F_ID_X_ADX_POS_CREATIVE_CPA));
    
    return;
}

} // namespace ors
} // namespace poseidon

