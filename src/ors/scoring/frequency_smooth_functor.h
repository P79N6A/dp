/**
**/

#ifndef _ORS_FREQUENCY_SMOOTH_FUNCTOR_H_
#define _ORS_FREQUENCY_SMOOTH_FUNCTOR_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/functor.h"

namespace poseidon
{
namespace ors
{

class FrequencySmoothFunctor : public Functor
{

public:
    FrequencySmoothFunctor();
    virtual ~FrequencySmoothFunctor();

    virtual bool Init();
    virtual void Fini();

    virtual int BeginWork(QueryAccessor* query_accessor);
    virtual int Work(AdAccessor* ad_accessor, QueryAccessor* query_accessor);
    virtual int EndWork(QueryAccessor* query_accessor);


protected:
    bool m_enable_frequency_smooth_flag;
    float m_frequency_smooth_factor;
    int m_creative_frequency_limit_cnt;
    int m_campaign_frequency_limit_cnt;
};

} // namespace ors
} // namespace poseidon

#endif // _ORS_USER_GRADE_FUNCTOR_H_

