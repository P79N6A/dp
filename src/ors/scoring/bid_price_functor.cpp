/**
**/

//include STD C/C++ head files
#include <algorithm>
#include <math.h>

//include third_party_lib head files
#include "src/ors/scoring/bid_price_functor.h"
#include "util/log.h"

namespace poseidon
{
namespace ors
{

BidPriceFunctor::BidPriceFunctor()
{
}

BidPriceFunctor::~BidPriceFunctor()
{

}

bool BidPriceFunctor::Init()
{
    return true;
}

void BidPriceFunctor::Fini()
{

}

int BidPriceFunctor::BeginWork(QueryAccessor* /*query_accessor*/)
{

    LOG_DEBUG("BidPriceFunctor BeginWork OK!");
    return 0;
}

float BidPriceFunctor::CalBidPrice(AdAccessor* ad_accessor)
{
    float ctr = ad_accessor->GetFeatureValueFloat(F_ID_X_AD_CTR);
    float cvr = ad_accessor->GetFeatureValueFloat(F_ID_X_AD_CVR);

    float price = 0.0f;

    float org_price = ad_accessor->GetFeatureValueFloat(F_ID_X_ORG_PRICE);
    if (ad_accessor->GetBillingType() == CPM) // CPM = 4
    {
       price = org_price; 
    }

    else if (ad_accessor->GetBillingType() == CPC) // CPC = 3
    {
        price = 1000 * ctr *  org_price;
    } 
    else if (ad_accessor->GetBillingType() == CPA) // CPA = 2
    {
        price = 1000 * ctr * cvr * org_price;
    }
    else if (ad_accessor->GetBillingType() == CPD) // CPD = 5
    {
        price = org_price;
    }

    price = price / (1.0f + ad_accessor->GetPremiumRate());
    LOG_DEBUG("ad_id=%d, billing_type=%d, org_price=%f, price=%f, premium_rate=%f, ctr=%f, cvr=%f",
            ad_accessor->GetAdId(), ad_accessor->GetBillingType(), org_price, price, 
            ad_accessor->GetPremiumRate(), ctr, cvr);

    return price;
}

int BidPriceFunctor::Work(AdAccessor* ad_accessor, QueryAccessor* /*query_accessor*/)
{
    float bid_price = this->CalBidPrice(ad_accessor);

    float ranking_score = ad_accessor->GetFeatureValueFloat(F_ID_X_AD_RANKING_SCORE) * bid_price;
    ad_accessor->SetFeature(F_ID_X_AD_RANKING_SCORE, ranking_score);

    ad_accessor->SetFeature(F_ID_X_AD_BID_PRICE, bid_price);

    LOG_DEBUG("ad_id=%d, bid_price=%f, ranking_score=%f",
               ad_accessor->GetAdId(), bid_price, ranking_score);

    return 0;
}

int BidPriceFunctor::EndWork(QueryAccessor* /*query_accessor*/)
{
    return 0;
}



} // namespace ors
} // namespace poseidon


