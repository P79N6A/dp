/**
**/

//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/reranking/external_bidding_functor.h"
#include "util/log.h"

namespace poseidon
{
namespace ors
{
ExternalBiddingFunctor::ExternalBiddingFunctor()
{

}
ExternalBiddingFunctor::~ExternalBiddingFunctor()
{

}

bool ExternalBiddingFunctor:: Init()
{
    m_enable_bidding_explore_flag = true;
    m_bidding_explore_rate = 0.001f;
    m_bidding_explore_price = 100.0f;

    m_enbale_random_bid_mode = false;
    return true;
}
void ExternalBiddingFunctor::Fini()
{

}

int ExternalBiddingFunctor::BeginWork(QueryAccessor* query_accessor)
{
    AdxBaseParam* base_param = query_accessor->GetAdxBaseParam(); 
    m_enable_bidding_explore_flag = true;
    m_bidding_explore_rate = 0.001f;
    m_bidding_explore_price = 100.0f;
    m_enbale_random_bid_mode = false;
    m_high_price_mode_id = 0;

    if (base_param->has_enable_bidding_explore_flag())
    {
        m_enable_bidding_explore_flag = base_param->enable_bidding_explore_flag();
    }
    if (base_param->has_bidding_explore_rate())
    {
        m_bidding_explore_rate = base_param->bidding_explore_rate();
    }
    if (base_param->has_bidding_explore_price())
    {
        m_bidding_explore_price = base_param->bidding_explore_price();
    }

    if (base_param->has_enable_random_bid_mode())
    {
        m_enbale_random_bid_mode = base_param->enable_random_bid_mode();
    }

    if (query_accessor->GetViewType() == 404) {
        m_high_price_mode_id = 1;
    }

    query_accessor->GetExpParam(EXP_PARAM_ORS_HIGH_PRICE_MODE_ID, &m_high_price_mode_id);
    m_bidding_proposal.Bind(query_accessor);

    LOG_DEBUG("enable_bidding_explore_flag=%d, bidding_explore_rate=%f,"
            "bidding_explore_price=%f, enbale_random_bid_mode=%d, high_price_mode_id=%d", 
            m_enable_bidding_explore_flag, m_bidding_explore_rate, 
            m_bidding_explore_price, m_enbale_random_bid_mode, m_high_price_mode_id);
    return 0;
}

void ExternalBiddingFunctor::ChooseHighPriceMode(QueryAccessor* query_accessor, AdAccessor* ad_accessor, int mode_id)
{
    if (mode_id == 1) {
        this->HighPriceModeOne(query_accessor, ad_accessor);
    }
}

void ExternalBiddingFunctor::HighPriceModeOne(QueryAccessor* query_accessor, AdAccessor* ad_accessor)
{
    // 高活跃/高付费，以及广告对该用户没有展示过频次，按cpm=bid_cost取bid_price出价
    int user_seed_active = query_accessor->GetTraceValueInterger(T_ID_USER_SEED_ID_ACTIVE);
    int user_seed_pay = query_accessor->GetTraceValueInterger(T_ID_USER_SEED_ID_PAY);

    int ad_user_freq = ad_accessor->GetAdUserFreq();
    float bid_price = ad_accessor->GetFeatureValueFloat(F_ID_X_AD_BID_PRICE);
    float org_bid_price = bid_price;

    if (ad_user_freq == 0 && 
        ((user_seed_pay >= 11 && user_seed_pay <= 16) || (user_seed_active>=21 && user_seed_active<=22))) {

        bid_price = m_bidding_proposal.GetBidCostPrice(ad_accessor->GetFeatureValueFloat(F_ID_X_AD_BID_PRICE));
        ad_accessor->SetFeature(F_ID_X_AD_BID_PRICE, bid_price);
    }

    LOG_DEBUG("ad=%d, user_seed_active=%d, user_seed_pay=%d, ad_user_freq=%d, bid_price=%f, org_bid_price=%f",
            ad_accessor->GetAdId(),user_seed_active,ad_user_freq,
            bid_price, org_bid_price);
}

AdAccessor* ExternalBiddingFunctor::RandomTopAd(AccessorProvider* accessor_provider)
{
    AdAccessor* select_ad_accessor = NULL; 
    if (accessor_provider->GetAdNum() > 0) {
        int idx = rand() % accessor_provider->GetAdNum();
        select_ad_accessor = accessor_provider->GetAdAccessor(idx);
        select_ad_accessor->SetFeature(F_ID_X_AD_FILTER, 0);
    }
    return select_ad_accessor;
}

AdAccessor* ExternalBiddingFunctor::SelectTopAd(CandidateAdSet* candidate_ad_set)
{
    float total_weight = 0.0f;
    AdAccessor* select_ad_accessor = NULL;
    int idx = 0;
    for (int i = 0; i < candidate_ad_set->GetInsertCount(); i++)
    {
        AdAccessor* ad_accessor = candidate_ad_set->GetCandidateAdAccessors(i);
        if (ad_accessor->GetFeatureValueInterger(F_ID_X_AD_IS_TOPN)) 
        {
            continue;
        }
        float rand_val = rand() / float(RAND_MAX);
        float weight = ad_accessor->GetFeatureValueFloat(F_ID_X_AD_TOPN_WEIGHT);
        total_weight += weight;
        if (rand_val * total_weight < weight)
        {
            select_ad_accessor = ad_accessor;
            idx = i;
        }
    }
    
    if (select_ad_accessor) {
        LOG_DEBUG("ad_id=%d, topn_weight=%f, total_weight=%f, idx=%d", 
            select_ad_accessor->GetAdId(), 
            select_ad_accessor->GetFeatureValueFloat(F_ID_X_AD_TOPN_WEIGHT),
            total_weight, idx);
    }

    return select_ad_accessor;
}

int ExternalBiddingFunctor::Work(AccessorProvider* accessor_provider)
{
    CandidateAdSet* candidate_ad_set = accessor_provider->GetCandidateAdSet();
    QueryAccessor* query_accessor = accessor_provider->GetQueryAccessor();

    LOG_DEBUG("adx_id=%d, pos_id=%s, candidate_ad_set count=%d, ad_num=%d",
            query_accessor->GetAdxId(), query_accessor->GetPosId().c_str(),
            candidate_ad_set->GetInsertCount(),
            accessor_provider->GetAdNum());

    if (!m_enbale_random_bid_mode && candidate_ad_set->GetInsertCount() == 0)
    {
        return 0;
    }

    TopnAdSet* topn_ad_set = accessor_provider->GetTopnAdSet();
    for (int i = 0; i < topn_ad_set->GetReqTopnCount(); i++) {
        AdAccessor* ad_accessor = this->SelectTopAd(candidate_ad_set);
        if (m_enbale_random_bid_mode) {
            ad_accessor = this->RandomTopAd(accessor_provider);
        }
        if (!ad_accessor)
        {
            break;
        }

        topn_ad_set->Insert(ad_accessor);
        ad_accessor->SetFeature(F_ID_X_AD_IS_TOPN, 1);
        LOG_DEBUG("ad_id=%d insert into topn_ad_set cnt=%d, req_top_count=%d",
                ad_accessor->GetAdId(),
                topn_ad_set->GetTopnCount(),
                topn_ad_set->GetReqTopnCount());
    }

    LOG_DEBUG("adx_id=%d, pos_id=%s, topn_ad_set count=%d",
            query_accessor->GetAdxId(), query_accessor->GetPosId().c_str(),
            topn_ad_set->GetTopnCount());

    if (topn_ad_set->GetTopnCount() == 0)
    {
        return 0;
    }

    float floor_price = query_accessor->GetFloorPrice();
    if (query_accessor->GetPosBaseParam()->has_bid_floor_price())
    {
        floor_price = query_accessor->GetPosBaseParam()->bid_floor_price();
        LOG_DEBUG("adx_id=%d, pos_id=%s, exp floor_price=%f",
                query_accessor->GetAdxId(), 
                query_accessor->GetPosId().c_str(),
                floor_price);
    }

    if (query_accessor->GetAdxId() == ADX_ID_YOUKU)
    {
        floor_price = std::min(floor_price, 6.0f);   
    }

    if (query_accessor->GetAdxId() == ADX_ID_IQIYI)
    {
        floor_price = std::min(floor_price, 4.0f);
    }

    for (int i = 0; i < topn_ad_set->GetTopnCount(); i++)
    {
        AdAccessor* ad_accessor = topn_ad_set->GetTopnAdAccessor(i);
        if (query_accessor->IsPdb())
        {
            ad_accessor->SetFeature(F_ID_X_AD_BID_PRICE, query_accessor->GetPdbInfo()->GetDealPrice());
            LOG_DEBUG("ad_id=%d, deal_id=%s, deal_price=%f, bid_price=%f", 
                    ad_accessor->GetAdId(),
                    query_accessor->GetPdbInfo()->GetDealId().c_str(), 
                    query_accessor->GetPdbInfo()->GetDealPrice(),
                    ad_accessor->GetFeatureValueFloat(F_ID_X_AD_BID_PRICE));
        }

        // 是否大于广告位底价
        if (ad_accessor->GetFeatureValueFloat(F_ID_X_AD_BID_PRICE) < floor_price)
        {
            ad_accessor->SetFeature(F_ID_X_AD_FILTER, 1);        
        }

        LOG_DEBUG("ad_id=%d, floor_price=%f, bid_price=%f, filter=%d",
                ad_accessor->GetAdId(), floor_price,
                ad_accessor->GetFeatureValueFloat(F_ID_X_AD_BID_PRICE),
                ad_accessor->GetFeatureValueInterger(F_ID_X_AD_FILTER));
        

        // 出价模式为固定出价
        if (ad_accessor->GetFeatureValueInterger(F_ID_X_BIDDING_MODE) == 1)
        {
            ad_accessor->SetFeature(F_ID_X_AD_BID_PRICE, ad_accessor->GetFeatureValueFloat(F_ID_X_BIDDING_FIXED_PRICE));
            LOG_DEBUG("ad_id=%d, bidding_fixed_price=%f",
                    ad_accessor->GetAdId(),
                    ad_accessor->GetFeatureValueFloat(F_ID_X_BIDDING_FIXED_PRICE));
        }
    }
    // 出价探测
    if (!query_accessor->IsPdb() && m_enable_bidding_explore_flag) {
        for (int  i = 0; i < topn_ad_set->GetTopnCount(); i++)
        {
            AdAccessor* ad_accessor = topn_ad_set->GetTopnAdAccessor(i);
            if (ad_accessor->GetFeatureValueInterger(F_ID_X_AD_FILTER) == 0 &&
                ad_accessor->GetFeatureValueInterger(F_ID_X_BIDDING_MODE) == 0)
            {

                if (m_high_price_mode_id > 0) {
                    LOG_DEBUG("exp high_price_mode_id=%d", m_high_price_mode_id);
                    this->ChooseHighPriceMode(query_accessor, ad_accessor, m_high_price_mode_id);
                }   
                if (rand() / float(RAND_MAX) < m_bidding_explore_rate)
                {
                    float explore_price = std::max(m_bidding_explore_price, ad_accessor->GetFeatureValueFloat(F_ID_X_AD_BID_PRICE));
                    ad_accessor->SetFeature(F_ID_X_AD_BID_PRICE, explore_price);
                    ad_accessor->SetFeature(F_ID_X_AD_BIDDING_EXPLORE_FLAG, 1);
                    query_accessor->SetTrace(T_ID_BIDDING_EXPLORE_FLAG, 1);
                }
                // 只对第一个广告做是否出价探测的判断
                break;
            }
        }
    }

    LOG_DEBUG("ExternalBiddingFunctor Work OK!explore=%d", 
            query_accessor->GetTraceValueInterger(T_ID_BIDDING_EXPLORE_FLAG));
    return 0;
}

int ExternalBiddingFunctor::EndWork(QueryAccessor* /*query_accessor*/)
{
    return 0;
}

} // namespace ors
} // namespace poseidon

