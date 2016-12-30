/**
**/

//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/bidding_proposal.h"
#include "src/model_updater/api/algo_model_data_api.h"
#include "util/log.h"

using namespace poseidon::model_updater;

namespace poseidon
{
namespace ors
{

BiddingProposal::BiddingProposal()
{

}

BiddingProposal::~BiddingProposal()
{

}


bool BiddingProposal::Bind(QueryAccessor* query_accessor)
{
    BiddingProposalKey key;
    key.pid = query_accessor->GetPosHashId();

    m_bidding_proposal_item.Clear();
    if (!AlgoModelDataApi::get_mutable_instance().GetBiddingProposalValue(key, &m_bidding_proposal_item)) {
        LOG_DEBUG("pid=%s GetBiddingProposalValue None", query_accessor->GetPosId().c_str());
    }  

    LOG_DEBUG("pid=%s GetBiddingProposalValue size=%d", 
            query_accessor->GetPosId().c_str(),
            m_bidding_proposal_item.approach_biddings_size());

    return true;
}


float BiddingProposal::GetBidCostPrice(float bid_cost)
{
    if (m_bidding_proposal_item.approach_biddings_size() == 0) {
        return bid_cost;
    }

    float price = bid_cost;
    for (int i = 0; i < m_bidding_proposal_item.approach_biddings_size(); i++) {
        const BiddingProposalItem::ApproachBidding& b = m_bidding_proposal_item.approach_biddings(i);
        price = b.bid_price();
        if (b.bid_cost() >= bid_cost) {
            break;
        }
    }
    return price;
}

} // namespace ors
} // namespace poseidon

