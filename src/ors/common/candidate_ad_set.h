/**
**/

#ifndef _ORS_CANDIDATE_AD_SET_H_
#define _ORS_CANDIDATE_AD_SET_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "util/random.h"
#include "src/ors/common/ad_accessor.h"
#include "src/ors/common/query_accessor.h"

namespace poseidon
{
namespace ors
{
class CandidateAdSet
{

public:
    CandidateAdSet();
    virtual ~CandidateAdSet();

    virtual bool Bind(QueryAccessor* query_accessor);
   
    bool Insert(AdAccessor* ad_accessor)
    {
        if (IsFull())
        {
            return false;
        }
        m_candidate_ads[m_index++] = ad_accessor;
        return true;
    }

    int GetInsertCount()
    {
        return m_index;
    }

    bool IsFull()
    {
        return m_index >= MAX_CANDIDATE_AD_NUM;
    }

    void Clear()
    {
        m_index = 0;
    }

    virtual AdAccessor* GetCandidateAdAccessors(int idx)
    {
        return m_candidate_ads[idx];
    }

protected:
    std::vector<AdAccessor*> m_candidate_ads;
    int m_index;
};

} // namespace ors
} // namespace poseidon

#endif // _ORS_CANDIDATE_AD_SET_H_

