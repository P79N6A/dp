/**
**/

#ifndef _ORS_EXTERNAL_BIDDING_FUNCTOR_H_
#define _ORS_EXTERNAL_BIDDING_FUNCTOR_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/functor.h"
#include "src/ors/common/bidding_proposal.h"

namespace poseidon
{
namespace ors
{

class ExternalBiddingFunctor : public Functor
{

public:
    ExternalBiddingFunctor();
    virtual ~ExternalBiddingFunctor();
    virtual bool Init();
    virtual void Fini();

    virtual int BeginWork(QueryAccessor* query_accessor);
    virtual int Work(AccessorProvider* accessor_provider);
    virtual int EndWork(QueryAccessor* query_accessor);

protected:
    virtual AdAccessor* SelectTopAd(CandidateAdSet* candidate_ad_set);
    virtual AdAccessor* RandomTopAd(AccessorProvider* accessor_provider);
    void HighPriceModeOne(QueryAccessor* QueryAccessor, AdAccessor* ad_accessor);
    void ChooseHighPriceMode(QueryAccessor* QueryAccessor, AdAccessor* ad_accessor, int mode_id);
protected:
    bool m_enable_bidding_explore_flag;
    float m_bidding_explore_rate;
    float m_bidding_explore_price;

    bool m_enbale_random_bid_mode;

    int m_high_price_mode_id;

    BiddingProposal m_bidding_proposal;
};
} // namespace ors
} // namespace poseidon

#endif // _ORS_EXTERNAL_BIDDING_FUNCTOR_H_

