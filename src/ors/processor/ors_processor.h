/**
**/

#ifndef _ORS_ORS_PROCESSOR_H_
#define _ORS_ORS_PROCESSOR_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/accessor_provider.h"
#include "src/ors/scoring/ctr_model_functor.h"
#include "src/ors/scoring/cvr_model_functor.h"
#include "src/ors/scoring/bid_price_functor.h"
#include "src/ors/scoring/cpa_promote_functor.h"
#include "src/ors/scoring/budget_pacing_functor.h"
#include "src/ors/scoring/frequency_smooth_functor.h"
#include "src/ors/scoring/bid_traffic_functor.h"
#include "src/ors/scoring/pay_factor_functor.h"
#include "src/ors/ranking/ranking_functor.h"
#include "src/ors/reranking/top_score_bidding_candidate.h"
#include "src/ors/reranking/long_tail_bidding_candidate.h"
#include "src/ors/reranking/budget_bidding_topn.h"
#include "src/ors/reranking/external_bidding_functor.h"


namespace poseidon
{
namespace ors
{
class OrsProcessor
{

public:
    OrsProcessor();
    virtual ~OrsProcessor();
    virtual int Init(const char* algo_conf_file, bool stat_on = false);
    virtual void Fini();
    
    virtual int Process(const AlgoRequest& algo_request, AlgoResponse* algo_response);
protected:
    virtual bool ProcessScoring(AccessorProvider* accessor_provider);
    virtual bool ProcessReranking(AccessorProvider* accessor_provider);
    virtual void WritePvLog(AccessorProvider* accessor_provider, AlgoResponse* algo_response);
    virtual void RecordToFeedback(AccessorProvider* accessor_provider, AlgoResponse* algo_response);
protected:
    AccessorProvider m_accessor_provider;
    CtrModelFunctor m_ctr_model_functor;
    CvrModelFunctor m_cvr_model_functor;
    BidPriceFunctor m_bid_price_functor;
    BidTrafficFunctor m_bid_traffic_functor;
    FrequencySmoothFunctor m_frequency_smooth_functor;
    CpaPromoteFunctor m_cpa_promote_functor;
    BudgetPacingFunctor m_budget_pacing_functor;
    PayFactorFunctor m_pay_factor_functor;
   
    RankingFunctor m_ranking_functor;

    TopScoreBiddingCandidate m_top_score_bidding_candidate;
    LongTailBiddingCandidate m_long_tail_bidding_candidate;
    BudgetBiddingTopn m_budget_bidding_topn;
    ExternalBiddingFunctor m_external_bidding_functor;

    std::vector<Functor*> m_scoring_functors;
    std::vector<Functor*> m_reranking_functors;
};
} // namespace ors
} // namespace poseidon

#endif // _ORS_ORS_PROCESSOR_H_

