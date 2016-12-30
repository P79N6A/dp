/**
**/

#ifndef _ORS_BUDGET_BIDDING_TOPN_H_
#define _ORS_BUDGET_BIDDING_TOPN_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/functor.h"

namespace poseidon
{
namespace ors
{
class BudgetBiddingTopn : public Functor
{

public:
    BudgetBiddingTopn();
    virtual ~BudgetBiddingTopn();
    virtual bool Init();
    virtual void Fini();

    virtual int BeginWork(QueryAccessor* query_accessor);
    virtual int Work(AccessorProvider* accessor_provider);
    virtual int EndWork(QueryAccessor* query_accessor);

protected:    
    bool m_enable_budget_bidding_flag;
};
} // namespace ors
} // namespace poseidon

#endif // _ORS_BUDGET_BIDDING_TOPN_H_

