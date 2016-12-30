/**
**/

#ifndef _ORS_BIDDING_PROPOSAL_H_
#define _ORS_BIDDING_PROPOSAL_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/query_accessor.h"


namespace poseidon
{
namespace ors
{
class BiddingProposal
{

public:
    BiddingProposal();
    virtual ~BiddingProposal();
    virtual bool Bind(QueryAccessor* query_accessor);
    float GetBidCostPrice(float bid_cost);
protected:
    BiddingProposalItem m_bidding_proposal_item;    
};
} // namespace ors
} // namespace poseidon

#endif // _ORS_BIDDING_PROPOSAL_H_

