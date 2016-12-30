/**
**/

//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/ranking/ranking_functor.h"
#include "util/log.h"

namespace poseidon
{
namespace ors
{

RankingFunctor::RankingFunctor()
{

}

RankingFunctor::~RankingFunctor()
{

}


bool RankingFunctor::Init()
{
    return true;
}


void RankingFunctor::Fini()
{

}

int RankingFunctor::BeginWork(QueryAccessor* query_accessor)
{
    m_enable_pdb_simple_mode = false;
    if (query_accessor->GetAdxBaseParam()->has_enable_pdb_simple_mode())
    {
        m_enable_pdb_simple_mode = query_accessor->GetAdxBaseParam()->enable_pdb_simple_mode();
    }

    m_bid_dev_id_set.clear();
    for (int i = 0; i < query_accessor->GetAdxBaseParam()->bid_dev_ids_size(); i++)
    {
        m_bid_dev_id_set.insert(query_accessor->GetAdxBaseParam()->bid_dev_ids(i));
        LOG_DEBUG("bid_dev_id=%s", query_accessor->GetAdxBaseParam()->bid_dev_ids(i).c_str());
    }

    LOG_DEBUG("RankingFunctor BeginWork OK!adx_id=%d, pos_id=%s, enable_pdb_simple_mode=%d",
            query_accessor->GetAdxId(), query_accessor->GetPosId().c_str(), m_enable_pdb_simple_mode);
    return 0;
}


int RankingFunctor::Work(AccessorProvider* accessor_provider)
{
    QueryAccessor* query_accessor = accessor_provider->GetQueryAccessor();
    LOG_DEBUG("adx_id=%d, pos_id=%s, AccessorProvider GetAdNum=%d",
            query_accessor->GetAdxId(), 
            query_accessor->GetPosId().c_str(), 
            accessor_provider->GetAdNum());

    if (accessor_provider->GetAdNum() == 0)
    {
        return 0;
    }

    for (int i = 0; i < accessor_provider->GetAdNum(); i++)
    {
        AdAccessor* ad_accessor = accessor_provider->GetAdAccessor(i);
        float ranking_rate = ad_accessor->GetFeatureValueFloat(F_ID_X_AD_RANKING_RATE);
        if (query_accessor->IsPdb() && m_enable_pdb_simple_mode)
        {
            float filling_rate = query_accessor->GetPdbInfo()->GetFillingRate() * 1.03f;
            float ad_quato = query_accessor->GetPdbInfo()->GetCampaignAdQuato(ad_accessor->GetCampaignId());
            ranking_rate = filling_rate * query_accessor->GetReqAdNum() / ad_quato;
            LOG_DEBUG("ad_id = %d, pdb filling_rate = %f, ad_quato=%f, ranking_rate=%f",
                    ad_accessor->GetAdId(), filling_rate, ad_quato, ranking_rate);
        }

        if (m_bid_dev_id_set.count(query_accessor->GetDeviceId()) > 0) {
            ranking_rate = 1.0f;
            LOG_DEBUG("ad_id = %d, bid dev_id=%s, ranking_rate=%f", 
                    ad_accessor->GetAdId(), query_accessor->GetDeviceId().c_str(), ranking_rate);
        }

        float rand_val = rand() / float(RAND_MAX);
        if (rand_val > ranking_rate)
        {
            ad_accessor->SetFeature(F_ID_X_AD_FILTER, 1);
        }
        LOG_DEBUG("ad_id=%d, ranking_rate=%f, rand_val=%f, filter=%d",
                ad_accessor->GetAdId(), ranking_rate, rand_val, 
                ad_accessor->GetFeatureValueInterger(F_ID_X_AD_FILTER));
        
    }

    accessor_provider->SortAdAccessors();

    LOG_DEBUG("RankingFunctor Work OK!");
    return 0;
}

int RankingFunctor::EndWork(QueryAccessor* /*query_accessor*/)
{
    return 0;
}


} // namespace ors
} // namespace poseidon

