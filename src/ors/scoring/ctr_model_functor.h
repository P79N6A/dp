/**
**/

#ifndef _ORS_CTR_MODEL_FUNCTOR_H_
#define _ORS_CTR_MODEL_FUNCTOR_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/functor.h"

namespace poseidon
{
namespace ors
{

class CtrModelFunctor : public Functor
{

public:
    CtrModelFunctor();
    virtual ~CtrModelFunctor();

    virtual bool Init();
    virtual void Fini();

    virtual int BeginWork(QueryAccessor* query_accessor);
    virtual int Work(AdAccessor* ad_accessor, QueryAccessor* query_accessor);
    virtual int EndWork(QueryAccessor* query_accessor);

protected:
    void SetLRQueryFea(QueryAccessor* query_accessor);
    void SetLRAdFea(AdAccessor* ad_accessor);
    float GetLRModelCtr(AdAccessor* ad_accessor);
    float GetStatRateModelCtr(AdAccessor* ad_accessor);

protected:
    float m_default_ctr;
    float m_min_ctr;
    bool m_enable_ctr_promote_flag;
    bool m_enable_ctr_lr_model_flag;
};

} // namespace ors
} // namespace poseidon

#endif // _ORS_CTR_MODEL_FUNCTOR_H_

