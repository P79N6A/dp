/**
**/

#ifndef _ORS_CPA_PROMOTE_FUNCTOR_H_
#define _ORS_CPA_PROMOTE_FUNCTOR_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/functor.h"

namespace poseidon
{
namespace ors
{

class CpaPromoteFunctor : public Functor
{

public:
    CpaPromoteFunctor();
    virtual ~CpaPromoteFunctor();

    virtual bool Init();
    virtual void Fini();

    virtual int BeginWork(QueryAccessor* query_accessor);
    virtual int Work(AdAccessor* ad_accessor, QueryAccessor* query_accessor);
    virtual int EndWork(QueryAccessor* query_accessor);


protected:
    bool m_enable_cpa_promote_flag;
    float m_cpa_promote_factor;
};

} // namespace ors
} // namespace poseidon

#endif // _ORS_CPA_PROMOTE_FUNCTOR_H_

