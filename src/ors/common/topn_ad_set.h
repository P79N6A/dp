/**
**/

#ifndef _ORS_TOPN_AD_SET_H_
#define _ORS_TOPN_AD_SET_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "util/random.h"
#include "src/ors/common/ad_accessor.h"
#include "src/ors/common/query_accessor.h"

namespace poseidon
{
namespace ors
{
class TopnAdSet
{

public:
    TopnAdSet();
    virtual ~TopnAdSet();

    virtual bool Bind(QueryAccessor* query_accessor);
   
    int GetTopnCount()
    {
        return m_index;
    }

    int GetReqTopnCount()
    {
        return m_req_topn_count;
    }

    void Clear()
    {
        m_index = 0;
    }


    virtual AdAccessor* GetTopnAdAccessor(int idx)
    {
        return m_topn_ads[idx];
    }

    virtual bool Insert(AdAccessor* ad_accessor)
    {
        if (m_index < m_req_topn_count)
        {
            m_topn_ads[m_index++] = ad_accessor;
            return true;
        }
        return false;
    }

protected:
    std::vector<AdAccessor*> m_topn_ads;
    int m_index;
    int m_req_topn_count;
};

} // namespace ors
} // namespace poseidon

#endif // _ORS_TOPN_AD_SET_H_

