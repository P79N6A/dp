/**
**/

#ifndef _ORS_CVR_MODEL_FUNCTOR_H_
#define _ORS_CVR_MODEL_FUNCTOR_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/functor.h"

namespace poseidon
{
namespace ors
{

class CvrModelFunctor : public Functor
{

public:
    CvrModelFunctor();
    virtual ~CvrModelFunctor();

    virtual bool Init();
    virtual void Fini();

    virtual int BeginWork(QueryAccessor* query_accessor);
    virtual int Work(AdAccessor* ad_accessor, QueryAccessor* query_accessor);
    virtual int EndWork(QueryAccessor* query_accessor);

protected:
    void SetLRQueryFea(QueryAccessor* query_accessor);
    void SetLRAdFea(AdAccessor* ad_accessor);
    float GetLRModelCtr(AdAccessor* ad_accessor);
    float GetStatRateModelCvr(AdAccessor* ad_accessor);

protected:
    float m_default_cvr;
    float m_min_cvr;
    bool m_enable_cvr_promote_flag;
    bool m_enable_cvr_lr_model_flag;
};

} // namespace ors
} // namespace poseidon

#endif // _ORS_CVR_MODEL_FUNCTOR_H_

