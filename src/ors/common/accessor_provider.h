/**
**/

#ifndef _ORS_ACCESSOR_PROVIDER_H_
#define _ORS_ACCESSOR_PROVIDER_H_
//include STD C/C++ head files
#include <algorithm>
#include <vector>

//include third_party_lib head files
#include "src/ors/common/common_def.h"
#include "src/ors/common/fixed_allocator.h"
#include "src/ors/common/ad_accessor.h"
#include "src/ors/common/query_accessor.h"
#include "src/ors/common/feature_data_assigner.h"
#include "src/ors/common/candidate_ad_set.h"
#include "src/ors/common/topn_ad_set.h"

namespace poseidon
{
namespace ors
{

class AccessorProvider
{

public:
    AccessorProvider();
    virtual ~AccessorProvider();
    
    virtual bool Init();
    virtual void Fini();

    virtual void Reset()
    {
        m_ad_allocator.Reset();
        m_ad_num = 0;
    }
    
    virtual bool BindAlgoRequest(const AlgoRequest& algo_request);
    virtual bool FillTopnAds(AlgoResponse* algo_response);
   
    virtual QueryAccessor* GetQueryAccessor()
    {
        return &m_query_accessor;
    }


    virtual AdAccessor* GetAdAccessor(int idx)
    {
        return m_ad_accessors[idx];
    }

    virtual AdAccessor* GetAdAccessorByAdId(uint32_t ad_id)
    {
        std::map<uint32_t, AdAccessor*>::iterator iter = m_ad_accessors_map.find(ad_id);
        if (iter == m_ad_accessors_map.end())
        {
            return NULL;
        }
        return iter->second;
    }

    int GetAdNum()
    {
        return m_ad_num;
    }

    void SortAdAccessors();
    virtual CandidateAdSet* GetCandidateAdSet()
    {
        return &m_candidate_ad_set;
    }

    virtual TopnAdSet* GetTopnAdSet()
    {
        return &m_topn_ad_set;
    }

protected:
    virtual bool BindAds(const AlgoRequest& algo_request);

protected:
    QueryAccessor m_query_accessor;
    FixedAllocator<AdAccessor> m_ad_allocator;
    std::vector<AdAccessor*> m_ad_accessors;
    std::map<uint32_t, AdAccessor*> m_ad_accessors_map;
    int m_ad_num;
    FeatureDataAssigner m_feature_data_assigner;
    CandidateAdSet m_candidate_ad_set;
    TopnAdSet m_topn_ad_set;
};

} // namespace ors
} // namespace poseidon

#endif // _ORS_ACCESSOR_PROVIDER_H_

