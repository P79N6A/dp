/**
**/

#ifndef _ORS_BUDGET_PACING_FUNCTOR_H_
#define _ORS_BUDGET_PACING_FUNCTOR_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/functor.h"

namespace poseidon
{
namespace ors
{
class BudgetPacingFunctor : public Functor
{

public:
    BudgetPacingFunctor();
    virtual ~BudgetPacingFunctor();
    virtual bool Init();
    virtual void Fini();

    virtual int BeginWork(QueryAccessor* query_accessor);
    virtual int Work(AdAccessor* ad_accessor, QueryAccessor* query_accessor);
    virtual int EndWork(QueryAccessor* query_accessor);

protected:
    bool m_enable_budget_pacing_flag;
    bool m_enable_pdb_budget_pacing_flag;
};
} // namespace ors
} // namespace poseidon

#endif // _ORS_BUDGET_PACING_FUNCTOR_H_

