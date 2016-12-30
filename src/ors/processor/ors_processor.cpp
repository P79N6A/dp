/**
**/

//include STD C/C++ head files
#include <stdlib.h>

//include third_party_lib head files
#include "src/ors/processor/ors_processor.h"
#include "src/ors/common/config_center.h"
#include "util/log.h"
#include "util/func.h"
#include "util/pvlog.h"
#include "src/monitor/api/monitor_api.h"
#include "util/date_time.h"
#include "src/ors/common/pro_stat.h"
#include "third_party/boost/include/boost/algorithm/string.hpp"
#include "src/model_updater/api/algo_model_data_api.h"

namespace poseidon
{
namespace ors
{

OrsProcessor::OrsProcessor()
{

}

OrsProcessor::~OrsProcessor()
{
    Fini();
}

int OrsProcessor::Init(const char* conf_path, bool stat_on)
{
    if (!ConfigCenter::get_mutable_instance().Init(conf_path))
    {
       LOG_ERROR("ConfigCenter Init Failed!conf_path=%s", conf_path);
       return -1;
    }

    if (!model_updater::AlgoModelDataApi::get_mutable_instance().Init())
    {
        LOG_ERROR("AlgoModelDataApi Init Failed!conf_path=%s", conf_path);
        return -1;
    }

    if (stat_on) {
        PRO_ON();
        LOG_INFO("PRO ON");
    }

    if (!m_accessor_provider.Init())
    {
        LOG_ERROR("AccessorProvider Init Failed!");
        return -2;
    }

    // Add Scoring Funcor Here
    // Please Attention the Sequence

    m_budget_pacing_functor.SetName("BudgetPacingFunctor");
    m_scoring_functors.push_back(&m_budget_pacing_functor);

    m_bid_traffic_functor.SetName("BidTrafficFunctor");
    m_scoring_functors.push_back(&m_bid_traffic_functor);
    
    m_ctr_model_functor.SetName("CtrModelFunctor");
    m_scoring_functors.push_back(&m_ctr_model_functor);

    m_cvr_model_functor.SetName("CvrModelFunctor");
    m_scoring_functors.push_back(&m_cvr_model_functor);

    m_bid_price_functor.SetName("BidPriceFunctor");
    m_scoring_functors.push_back(&m_bid_price_functor);

    m_frequency_smooth_functor.SetName("FrequencySmoothFunctor");
    m_scoring_functors.push_back(&m_frequency_smooth_functor);

    m_cpa_promote_functor.SetName("CpaPromoteFunctor");
    m_scoring_functors.push_back(&m_cpa_promote_functor);

    m_pay_factor_functor.SetName("PayFactorFunctor");
    m_scoring_functors.push_back(&m_pay_factor_functor);

    for (size_t i = 0; i < m_scoring_functors.size(); i++)
    {
        if(!m_scoring_functors[i]->Init())
        {
              LOG_ERROR("%s Init Failed!", m_scoring_functors[i]->GetName().c_str());
              return -3;
        }
    }

    // Add RerankingFunctor Here
    // Please Attention the Sequence
    
    m_ranking_functor.SetName("RankingFunctor");
    m_reranking_functors.push_back(&m_ranking_functor);
    m_top_score_bidding_candidate.SetName("TopScoreBiddingCandidate");
    m_reranking_functors.push_back(&m_top_score_bidding_candidate);
    m_long_tail_bidding_candidate.SetName("LongTailBiddingCandidate");
    m_reranking_functors.push_back(&m_long_tail_bidding_candidate);

    m_budget_bidding_topn.SetName("BudgetBiddingTopn");
    m_reranking_functors.push_back(&m_budget_bidding_topn);

    m_external_bidding_functor.SetName("ExternalBiddingFunctor");
    m_reranking_functors.push_back(&m_external_bidding_functor);
    for (size_t i = 0; i < m_reranking_functors.size(); i++)
    {
        if(!m_reranking_functors[i]->Init())
        {
              LOG_ERROR("%s Init Failed!", m_reranking_functors[i]->GetName().c_str());
              return -4;
        }
    }


    LOG_DEBUG("OrsProcessor Init OK!");
    return 0;
}

void OrsProcessor::Fini()
{
    for (size_t i = 0; i < m_scoring_functors.size(); i++)
    {
        m_scoring_functors[i]->Fini();
    }

    for (size_t i = 0; i < m_reranking_functors.size(); i++)
    {
        m_reranking_functors[i]->Fini();
    }
}


int OrsProcessor::Process(const AlgoRequest& algo_request, AlgoResponse* algo_response)
{
    util::DateTime::get_mutable_instance().Now();
    PRO_CLEAR();

    // 提取特征信息
    if (!m_accessor_provider.BindAlgoRequest(algo_request))
    {
        LOG_ERROR("AccessorProviver BindAlgoRequest Failed!");
        return -1;
    }
    
    // Scoring
    if (!this->ProcessScoring(&m_accessor_provider))
    {
        LOG_ERROR("ProcessScoring Failed!");
        return -2;
    }

    // Reranking
    if (!this->ProcessReranking(&m_accessor_provider))
    {
        LOG_ERROR("ProcessReranking Failed!");
        return -3;
    }

    //填充topn 广告
    m_accessor_provider.FillTopnAds(algo_response);

    this->RecordToFeedback(&m_accessor_provider, algo_response);
    // 写日志
    this->WritePvLog(&m_accessor_provider, algo_response);

    LOG_DEBUG("OrsProcessor Process OK!");
    return 0;
}

void OrsProcessor::RecordToFeedback(AccessorProvider* accessor_provider, AlgoResponse* algo_response)
{
    QueryAccessor* query_accessor = accessor_provider->GetQueryAccessor();
    
    util::KeyValue* user_seed = algo_response->add_algo_feedbacks();
    user_seed->set_key("user_seed");
    user_seed->set_value(util::Func::to_str(query_accessor->GetTraceValueInterger(T_ID_USER_SEED_CATE_ACTIVE)));

    util::KeyValue* user_grade = algo_response->add_algo_feedbacks();
    user_grade->set_key("user_grade");
    user_grade->set_value(util::Func::to_str(query_accessor->GetTraceValueInterger(T_ID_USER_GRADE)));

    util::KeyValue* context_grade = algo_response->add_algo_feedbacks();
    context_grade->set_key("context_quality");
    if (query_accessor->GetTraceValueFloat(T_ID_CONTEXT_GRADE_QUALITY) >= FZERO) 
    {
        context_grade->set_value(util::Func::to_str(int(query_accessor->GetTraceValueFloat(T_ID_CONTEXT_GRADE_QUALITY)*100)));
    }
    else
    {
        context_grade->set_value("-1");
    }

    if (query_accessor->IsVideo()) {
        VideoInfo* video_info = query_accessor->GetVideoInfo();
        
        util::KeyValue* title = algo_response->add_algo_feedbacks();
        title->set_key("title");
        title->set_value(video_info->GetTitleStr());

        util::KeyValue* keywords = algo_response->add_algo_feedbacks();
        keywords->set_key("keywords");
        keywords->set_value(boost::join(video_info->GetKeywordsStr(), "|"));

        util::KeyValue* vid = algo_response->add_algo_feedbacks();
        vid->set_key("vid");
        vid->set_value(video_info->GetVIdStr());

        util::KeyValue* show_id = algo_response->add_algo_feedbacks();
        show_id->set_key("show_id");
        show_id->set_value(video_info->GetShowIdStr());

        util::KeyValue* cnl = algo_response->add_algo_feedbacks();
        cnl->set_key("cnl");
        cnl->set_value(video_info->GetFChannelStr());

        util::KeyValue* cnl2 = algo_response->add_algo_feedbacks();
        cnl2->set_key("cnl2");
        cnl2->set_value(boost::join(video_info->GetSChannelsStr(), "|"));

        util::KeyValue* video_owner = algo_response->add_algo_feedbacks();
        video_owner->set_key("video_owner");
        video_owner->set_value(video_info->GetOwnerIdStr());
    }

    algo_response->add_algo_pvlogs()->CopyFrom(*user_seed);
    algo_response->add_algo_pvlogs()->CopyFrom(*user_grade);
    algo_response->add_algo_pvlogs()->CopyFrom(*context_grade);   
}


void OrsProcessor::WritePvLog(AccessorProvider* accessor_provider, AlgoResponse* algo_response)
{
    std::stringstream ss;
    #define PV_SET(name, val) ss<<(#name)<<"="<<(val)<<"`"
    #define PV_NSET(name, val) ss<<(name)<<"="<<(val)<<"`"
    std::string dt;
    util::Func::get_time_str(&dt);
    PV_SET(dt, util::DateTime::get_mutable_instance().GetTimeStr());

    QueryAccessor* query_accessor = accessor_provider->GetQueryAccessor();

    PV_SET(adx_id, query_accessor->GetAdxId());
    PV_SET(pos_id, query_accessor->GetPosId());
    PV_SET(view_type, query_accessor->GetViewType());
    PV_SET(pos_size, query_accessor->GetPosSize());
    PV_SET(app, query_accessor->GetAppName());
    PV_SET(device_id, query_accessor->GetDeviceId());
    PV_SET(os, query_accessor->GetOs());
    if (query_accessor->IsPdb())
    {
        PV_SET(deal_id, query_accessor->GetPdbInfo()->GetDealId());
    }

    for (int i = 0; i < algo_response->algo_feedbacks_size();i++)
    {
        PV_NSET(algo_response->algo_feedbacks(i).key(), algo_response->algo_feedbacks(i).value());
    }

    if (query_accessor->IsVideo()) 
    {
        if(!query_accessor->GetTraceValue(T_ID_CONTEXT_VIDEO_GRADE_QUALITY).IsNull()) {
            PV_SET(video_quality, query_accessor->GetTraceValueFloat(T_ID_CONTEXT_VIDEO_GRADE_QUALITY));
        }
        else 
        {
            PV_SET(video_quality, -1);
        }
    }

    if(!query_accessor->GetTraceValue(T_ID_CONTEXT_SPOT_GRADE_QUALITY).IsNull()) {
        PV_SET(spot_quality, query_accessor->GetTraceValueFloat(T_ID_CONTEXT_SPOT_GRADE_QUALITY));
    }
    else
    {
        PV_SET(spot_quality, -1);
    }
    
    PV_SET(req_ad_num, query_accessor->GetReqAdNum());
    PV_SET(floor_price, query_accessor->GetFloorPrice());
    PV_SET(bidding_explore, query_accessor->GetTraceValueInterger(T_ID_BIDDING_EXPLORE_FLAG));

    TopnAdSet* topn_ad_set = accessor_provider->GetTopnAdSet();
    PV_SET(algo_topn, topn_ad_set->GetTopnCount()); 
    PV_SET(rsp_topn, algo_response->algoed_ads_size());
    if (algo_response->algoed_ads_size() > 0)
    {
        PRO_STAT(TPROPERTY_ID_POS_RSP, query_accessor->GetPosHashId(), algo_response->algoed_ads_size());
        PRO_STAT(TPROPERTY_ID_ADX_RSP, query_accessor->GetAdxId(), algo_response->algoed_ads_size());
        if (query_accessor->IsPdb())
        {
            PRO_STAT(TPROPERTY_ID_DEAL_RSP, query_accessor->GetPdbInfo()->GetHashDealId(), algo_response->algoed_ads_size());
        }
    }
    for (int i = 0; i < topn_ad_set->GetTopnCount(); i++)
    {
        AdAccessor* ad_accessor = topn_ad_set->GetTopnAdAccessor(i);
        PV_SET(ad_id, ad_accessor->GetAdId());
        PV_SET(campaign_id, ad_accessor->GetCampaignId());
        PV_SET(price, ad_accessor->GetFeatureValueFloat(F_ID_X_AD_BID_PRICE));
        PV_SET(org_price, ad_accessor->GetFeatureValueFloat(F_ID_X_ORG_PRICE));
        PV_SET(billing_type, ad_accessor->GetBillingType());
    }

    PVLOG(ss.str());

    if (topn_ad_set->GetTopnCount() > 0)
    {
        MON_ADD(ATTR_SVR_ORS_HAS_ALGO_TOPN_COUNT, 1);
    }
    else 
    {
        MON_ADD(ATTR_SVR_ORS_NOT_ALGO_TOPN_COUNT, 1);
    }

}

bool OrsProcessor::ProcessScoring(AccessorProvider* accessor_provider)
{
    int ret = 0;
    QueryAccessor* query_accessor = accessor_provider->GetQueryAccessor();
    for (size_t i = 0; i < m_scoring_functors.size(); i++)
    {
        ret = m_scoring_functors[i]->BeginWork(query_accessor);
        if (ret != 0)
        {
              LOG_ERROR("%s BeginWork Failed!", m_scoring_functors[i]->GetName().c_str());
              return false;
        }

        for (int j = 0; j < accessor_provider->GetAdNum(); j++)
        {
            AdAccessor* ad_accessor = accessor_provider->GetAdAccessor(j);
            ret = m_scoring_functors[i]->Work(ad_accessor, query_accessor);
            if (ret != 0)
            {
                LOG_ERROR("%s Work Failed!ad_id=%d", 
                        m_scoring_functors[i]->GetName().c_str(),
                        ad_accessor->GetAdId());
                return false;
            }
        }
        
        ret = m_scoring_functors[i]->EndWork(query_accessor);
        if (ret != 0)
        {
              LOG_ERROR("%s EndWork Failed!", m_scoring_functors[i]->GetName().c_str());
              return false;
        }
        LOG_DEBUG("ScoringFunctor %s Process OK!", m_scoring_functors[i]->GetName().c_str());
    }

    LOG_DEBUG("OrsProcessor BeginScoring OK!");
    return true;
}

bool OrsProcessor::ProcessReranking(AccessorProvider* accessor_provider)
{
    int ret = 0;
    QueryAccessor* query_accessor = accessor_provider->GetQueryAccessor();
    for (size_t i = 0; i < m_reranking_functors.size(); i++)
    {
        ret = m_reranking_functors[i]->BeginWork(query_accessor);
        if (ret != 0)
        {
              LOG_ERROR("%s BeginWork Failed!", m_reranking_functors[i]->GetName().c_str());
              return false;
        }

        ret = m_reranking_functors[i]->Work(accessor_provider);
        if (ret != 0)
        {
              LOG_ERROR("%s Work Failed!", m_reranking_functors[i]->GetName().c_str());
              return false;
        }

        ret = m_reranking_functors[i]->EndWork(query_accessor);
        if (ret != 0)
        {
              LOG_ERROR("%s EndWork Failed!", m_reranking_functors[i]->GetName().c_str());
              return false;
        }

        LOG_DEBUG("RerankingFunctor %s Process OK!", m_reranking_functors[i]->GetName().c_str());
    }

    LOG_DEBUG("OrsProcessor ProcessReranking OK!");
    return true;
}

} // namespace ors
} // namespace poseidon

