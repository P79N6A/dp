/**
**/

//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/candidate_ad_set.h"
#include "util/log.h"

namespace poseidon
{
namespace ors
{

CandidateAdSet::CandidateAdSet()
{
    m_candidate_ads.resize(MAX_CANDIDATE_AD_NUM);
    m_index = 0;
}

CandidateAdSet::~CandidateAdSet()
{

}

bool CandidateAdSet::Bind(QueryAccessor* /*query_accessor*/)
{
    m_index = 0;

    return true;
}


} // namespace ors
} // namespace poseidon

