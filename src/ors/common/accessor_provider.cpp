/**
**/

//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/accessor_provider.h"
#include "src/ors/common/config_center.h"
#include "util/log.h"
#include "src/ors/common/strategy.h"

namespace poseidon
{
namespace ors
{

AccessorProvider::AccessorProvider()
{

}

AccessorProvider::~AccessorProvider()
{
    
}
bool AccessorProvider::Init()
{
    m_ad_num = 0;
    m_ad_allocator.SetQuato(MAX_AD_NUM);
    m_ad_accessors.resize(MAX_AD_NUM);


    if (!m_feature_data_assigner.Init())
    {
        LOG_ERROR("FeatureDataAssigner Init Failed!");
        return false;
    }

    LOG_INFO("AccessorProvider Init OK");

    return true;
}

void AccessorProvider::Fini()
{
    m_feature_data_assigner.Fini();
}


bool AccessorProvider::BindAlgoRequest(const AlgoRequest& algo_request)
{
    this->Reset();

    int ret = m_query_accessor.Bind(algo_request);
    if (ret != 0)
    {
        LOG_ERROR("QueryAccessor Bind Failed! ret = %d", ret);
        return false;
    }

    if (!m_feature_data_assigner.Bind(&m_query_accessor))
    {
        LOG_ERROR("FeatureDataAssigner Bind Failed!");
        return false;
    }

    if (!BindAds(algo_request))
    {
        LOG_ERROR("BindAds Failed!");
        return false;
    }

    if (!m_candidate_ad_set.Bind(&m_query_accessor))
    {
        LOG_ERROR("CandidateAdSet Bind Failed!");
        return false;
    }
    
    if (!m_topn_ad_set.Bind(&m_query_accessor))
    {
        LOG_ERROR("TopnAdSet Bind Faild!");
        return false;
    }

    return true;
}

void AccessorProvider::SortAdAccessors()
{
    std::sort(m_ad_accessors.begin(), m_ad_accessors.begin() + m_ad_num, AdCmp);
}


bool AccessorProvider::FillTopnAds(AlgoResponse* algo_response)
{
    for (int i = 0; i < m_topn_ad_set.GetTopnCount(); i++)
    {
        AdAccessor* ad_accessor = m_topn_ad_set.GetTopnAdAccessor(i);
        if (ad_accessor->GetFeatureValueInterger(F_ID_X_AD_FILTER) == 1)
        {
            continue;
        }
        AlgoedAd* ad = algo_response->add_algoed_ads();
        ad->set_id(ad_accessor->GetIndexId());
        ad->set_algo_price(100 * ad_accessor->GetFeatureValueFloat(F_ID_X_AD_BID_PRICE));
        ad->set_cost_price(ad_accessor->GetOrgPrice());
        ad->set_traffic_bid_flag(ad_accessor->GetFeatureValueInterger(F_ID_X_AD_BIDDING_EXPLORE_FLAG));
    }

    LOG_DEBUG("FillTopnAds Finish!rsp_ad_cnt=%d", algo_response->algoed_ads_size());
    return true;
}

bool AccessorProvider::BindAds(const AlgoRequest& algo_request)
{
    int ret = 0;
    m_ad_num = 0;
    m_ad_accessors_map.clear();

    for (int i = 0; i < algo_request.algo_ads_size(); i++)
    {
        AdAccessor* ad_accessor = m_ad_allocator.AllocItem();
        if (ad_accessor == NULL) 
        {
            LOG_WARN("Alloc AdAccessor idx=%d Failed!", i);
            continue;
        }
        ret = ad_accessor->Bind(algo_request.algo_ads(i));
        if (ret != 0)
        {
            LOG_ERROR("AdAccessor Bind Failed!ret=%d, idx=%d", ret, i);
            return false;
        }

        if (!m_feature_data_assigner.Assign(ad_accessor))
        {
            LOG_ERROR("AdAccessor Bind Failed!");
            return false;
        }

        if (m_ad_num >= MAX_AD_NUM)
        {
            LOG_WARN("Attention:ad size beyond limit");
            break;
        }
        m_ad_accessors[m_ad_num++] = ad_accessor;
        m_ad_accessors_map[ad_accessor->GetAdId()] = ad_accessor;
    }

    for (int i = 0; i < algo_request.feedbacks_size(); i++)
    {
        uint32_t ad_id = algo_request.feedbacks(i).adgroup_id();
        AdAccessor* ad_accessor = this->GetAdAccessorByAdId(ad_id);
        if (ad_accessor)
        {
            ret = ad_accessor->Bind(algo_request.feedbacks(i));
            if (ret != 0)
            {
                LOG_ERROR("AdAccessor Bind Failed!ret=%d, ad_id=%d", ret, ad_id);
                return false;
            }
        }
    }

    LOG_DEBUG("BindAds OK!Ad Num = %d", m_ad_num);
    return true;
}

} // namespace ors
} // namespace poseidon

