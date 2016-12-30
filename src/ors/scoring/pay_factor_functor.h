/**
**/

#ifndef _ORS_PAY_FACTOR_FUNCTOR_H_
#define _ORS_PAY_FACTOR_FUNCTOR_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/functor.h"

namespace poseidon
{
namespace ors
{

class PayFactorFunctor : public Functor
{

public:
    PayFactorFunctor();
    virtual ~PayFactorFunctor();

    virtual bool Init();
    virtual void Fini();

    virtual int BeginWork(QueryAccessor* query_accessor);
    virtual int Work(AdAccessor* ad_accessor, QueryAccessor* query_accessor);
    virtual int EndWork(QueryAccessor* query_accessor);

protected:
    bool m_enable_pay_factor_flag;
};

} // namespace ors
} // namespace poseidon

#endif // _ORS_PAY_FACTOR_FUNCTOR_H_

