/**
**/

#ifndef _ORS_FEATURE_DATA_ASSIGNER_H_
#define _ORS_FEATURE_DATA_ASSIGNER_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/query_accessor.h"
#include "src/ors/common/ad_accessor.h"

namespace poseidon
{
namespace ors
{
class FeatureDataAssigner
{

public:
    FeatureDataAssigner();
    virtual ~FeatureDataAssigner();

    virtual bool Init();
    virtual void Fini();
    virtual bool Bind(QueryAccessor* query_accessor);
    virtual bool Assign(AdAccessor* ad_accessor);
protected:
    void AssignStatRateFeature(AdAccessor* ad_accessor);
    void AssinBudgetPacingFeature(AdAccessor* ad_accessor);
    void AssignVideoContextGradeFeature(QueryAccessor* query_accessor);
    void AssignSpotGradeFeature(QueryAccessor* query_accessor);
    void AssignBaseParam(QueryAccessor* query_accessor);
    void AssignPayFactorFeature(AdAccessor* ad_accessor);
protected:
    uint32_t m_adx_id;
    uint64_t m_pos_id;
    uint32_t m_view_type;
    uint32_t m_os_type;
};
} // namespace ors
} // namespace poseidon

#endif // _ORS_FEATURE_DATA_ASSIGNER_H_

