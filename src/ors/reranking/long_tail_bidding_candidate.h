/**
**/

#ifndef _ORS_LONG_TAIL_BIDDING_CANDIDATE_H_
#define _ORS_LONG_TAIL_BIDDING_CANDIDATE_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/functor.h"

namespace poseidon
{
namespace ors
{
class LongTailBiddingCandidate : public Functor
{

public:
    LongTailBiddingCandidate();
    virtual ~LongTailBiddingCandidate();
    virtual bool Init();
    virtual void Fini();

    virtual int BeginWork(QueryAccessor* query_accessor);
    virtual int Work(AccessorProvider* accessor_provider);
    virtual int EndWork(QueryAccessor* query_accessor);

protected:    
    bool m_enable_long_tail_bidding_flag;
    uint32_t m_candidate_min_ad_num;

};
} // namespace ors
} // namespace poseidon

#endif // _ORS_LONG_TAIL_BIDDING_CANDIDATE_H_

