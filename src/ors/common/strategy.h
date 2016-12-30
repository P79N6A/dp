/**
**/

#ifndef _ORS_STRATEGY_H_
#define _ORS_STRATEGY_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/ad_accessor.h"

namespace poseidon
{
namespace ors
{

static bool AdCmp(AdAccessor* lhs, AdAccessor* rhs)
{
    int lhs_filter = lhs->GetFeatureValueInterger(F_ID_X_AD_FILTER);
    int rhs_filter = rhs->GetFeatureValueInterger(F_ID_X_AD_FILTER);

    float lhs_score = lhs->GetFeatureValueFloat(F_ID_X_AD_RANKING_SCORE);
    float rhs_score = rhs->GetFeatureValueFloat(F_ID_X_AD_RANKING_SCORE);

    int lhs_id = lhs->GetAdId();
    int rhs_id = rhs->GetAdId();

    return (lhs_filter < rhs_filter)
        || (lhs_filter == rhs_filter && lhs_score > rhs_score)
        || (lhs_filter == rhs_filter && lhs_score == rhs_score && lhs_id > rhs_id);
}

} // namespace ors
} // namespace poseidon

#endif // _ORS_STRATEGY_H_

