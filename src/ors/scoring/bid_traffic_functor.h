/**
**/

#ifndef _ORS_BID_TRAFFIC_FUNCTOR_H_
#define _ORS_BID_TRAFFIC_FUNCTOR_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/functor.h"

namespace poseidon
{
namespace ors
{

class BidTrafficFunctor : public Functor
{
public:
    BidTrafficFunctor();
    virtual ~BidTrafficFunctor();

    virtual bool Init();
    virtual void Fini();

    virtual int BeginWork(QueryAccessor* query_accessor);
    virtual int Work(AdAccessor* ad_accessor, QueryAccessor* query_accessor);
    virtual int EndWork(QueryAccessor* query_accessor);
protected:
    bool SetupSeedUser(QueryAccessor* query_accessor);
    bool SetupContextGrade(QueryAccessor* query_accessor);
    bool SetupUserGrade(QueryAccessor* query_accessor);
    bool SetupSeedUserNew(QueryAccessor* query_accessor);

    int GetSeedUserCode(int seed_cate)
    {
        switch(seed_cate)
        {
            case 1001:
                return 1;
            case 1101:
                return 2;
            case 1102:
                return 3;
            case 1103:
                return 4;
        }
        return 0;
    }
protected:
    bool m_enable_context_grade_flag;
    float m_ranking_rate_context_grade_factor;
    float m_context_grade_threshold;
    float m_context_grade_promote_rate_factor;
    bool m_context_grade_filter_flag;

    bool m_enable_user_grade_flag;
    float m_ranking_rate_user_grade_factor;
    int m_user_grade_threshold;
    float m_user_grade_promote_rate_factor;
    int m_user_grade_plan_id;
    bool m_user_grade_filter_flag;

    bool m_enable_seed_user_promote_flag;
    float m_ranking_rate_seed_user_factor;
    int m_seed_user_threshold;
    float m_seed_user_base_promote_rate;
    float m_seed_user_promote_rate_factor;
    bool m_seed_user_filter_flag;
};
} // namespace ors
} // namespace poseidon

#endif // _ORS_CONTEXT_GRADE_FUNCTOR_H_

