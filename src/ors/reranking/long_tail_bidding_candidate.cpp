/**
**/

//include STD C/C++ head files
#include <math.h>

//include third_party_lib head files
#include "src/ors/reranking/long_tail_bidding_candidate.h"
#include "util/log.h"

namespace poseidon
{
namespace ors
{
LongTailBiddingCandidate::LongTailBiddingCandidate()
{

}
LongTailBiddingCandidate::~LongTailBiddingCandidate()
{

}

bool LongTailBiddingCandidate:: Init()
{
    m_enable_long_tail_bidding_flag = true;
    m_candidate_min_ad_num = 3;
    return true;
}

void LongTailBiddingCandidate::Fini()
{

}

int LongTailBiddingCandidate::BeginWork(QueryAccessor* query_accessor)
{
    m_enable_long_tail_bidding_flag = true;
    m_candidate_min_ad_num = 3;
 
    AdxBaseParam* base_param = query_accessor->GetAdxBaseParam(); 
    if (base_param->has_enable_long_tail_bidding_flag())
    {
        m_enable_long_tail_bidding_flag = base_param->enable_long_tail_bidding_flag();
        if (base_param->has_candidate_min_ad_num())
        {
            m_candidate_min_ad_num = base_param->candidate_min_ad_num();
        }
    }

    m_candidate_min_ad_num = std::max(m_candidate_min_ad_num, query_accessor->GetReqAdNum());

    LOG_DEBUG("adx_id=%d, pos_id=%s, enable_long_tail_bidding_flag=%d, candidate_min_ad_num=%d, req_ad_num=%d",
            query_accessor->GetAdxId(), query_accessor->GetPosId().c_str(),
            m_enable_long_tail_bidding_flag, m_candidate_min_ad_num,
            query_accessor->GetReqAdNum());
    return 0;
}

int LongTailBiddingCandidate::Work(AccessorProvider* accessor_provider)
{
    if (!m_enable_long_tail_bidding_flag)
    {
        return 0;
    }

    QueryAccessor* query_accessor = accessor_provider->GetQueryAccessor();
    LOG_DEBUG("adx_id=%d, pos_id=%s, AccessorProvider GetAdNum=%d",
            query_accessor->GetAdxId(), 
            query_accessor->GetPosId().c_str(), 
            accessor_provider->GetAdNum());

    if (accessor_provider->GetAdNum() == 0)
    {
        return 0;
    }

    CandidateAdSet* candidate_ad_set = accessor_provider->GetCandidateAdSet();
    if (candidate_ad_set->GetInsertCount() >= (int)m_candidate_min_ad_num)
    {
        LOG_DEBUG("adx_id=%d, pos_id=%s,candidate_ad_set ad_num=%d >= candidate_min_ad_num=%d",
                query_accessor->GetAdxId(), query_accessor->GetPosId().c_str(),
                candidate_ad_set->GetInsertCount(), m_candidate_min_ad_num);
        return 0;
    }

    for (int i = 0; i < accessor_provider->GetAdNum(); i++)
    {
        AdAccessor* ad_accessor = accessor_provider->GetAdAccessor(i);
        if (ad_accessor->GetFeatureValueInterger(F_ID_X_AD_FILTER) == 1)
        {
            break;
        }

        if (ad_accessor->GetFeatureValueInterger(F_ID_X_AD_IS_CANDIDATE))
        {
            continue;
        }

        if (!candidate_ad_set->Insert(ad_accessor))
        {
            LOG_WARN("Attention:candidate_ad_set beyond limit");
            break;
        }

        ad_accessor->SetFeature(F_ID_X_AD_IS_CANDIDATE, 1);

        LOG_DEBUG("ad_id=%d insert into candidate_ad_set,candidate_ad_set ad_num=%d <= candidate_min_ad_num=%d", 
                    ad_accessor->GetAdId(), candidate_ad_set->GetInsertCount(), m_candidate_min_ad_num);
        if (candidate_ad_set->GetInsertCount() >= (int)m_candidate_min_ad_num)
        {
            break;
        }
    }

    LOG_DEBUG("adx_id=%d, pos_id=%s, candidate_ad_set count=%d", 
            query_accessor->GetAdxId(), query_accessor->GetPosId().c_str(),
            candidate_ad_set->GetInsertCount());

    return 0;
}

int LongTailBiddingCandidate::EndWork(QueryAccessor* /*query_accessor*/)
{
    return 0;
}


} // namespace ors
} // namespace poseidon

