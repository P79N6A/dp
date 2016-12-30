/**
**/

//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/topn_ad_set.h"
#include "util/log.h"

namespace poseidon
{
namespace ors
{

TopnAdSet::TopnAdSet()
{
    m_index = 0;
}

TopnAdSet::~TopnAdSet()
{

}

bool TopnAdSet::Bind(QueryAccessor* query_accessor)
{
    m_index = 0;
    m_req_topn_count = query_accessor->GetReqAdNum();
    m_topn_ads.clear();
    m_topn_ads.resize(m_req_topn_count);

    return true;
}


} // namespace ors
} // namespace poseidon

