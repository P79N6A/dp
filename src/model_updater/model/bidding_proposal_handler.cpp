/**
**/

//include STD C/C++ head files
#include <fstream>

//include third_party_lib head files
#include "src/model_updater/model/bidding_proposal_handler.h"
#include "src/model_updater/api/structs.h"
#include "third_party/cityhash/include/city.h"
#include "util/log.h"
#include "util/proto_helper.h"
#include "util/func.h"


namespace poseidon
{
namespace model_updater
{

BiddingProposalHandler::BiddingProposalHandler()
{

}

BiddingProposalHandler::~BiddingProposalHandler()
{

}

bool BiddingProposalHandler::Update()
{
    ors::AlgoModelData algo_model_data;
    if(!util::ParseProtoFromBinaryFormatFile(m_watch_file.c_str(), algo_model_data.mutable_bidding_proposal_model_data()))
    {
        LOG_ERROR("ParseProtoFromTextFormatFile %s Failed!", m_watch_file.c_str());
        return false;
    }

    BiddingProposalKey key;
    const ors::BiddingProposalModel& bidding_proposal_model = algo_model_data.bidding_proposal_model_data();
    for (int i = 0; i < bidding_proposal_model.items_size(); i++)
    {
        const ors::BiddingProposalItem& item = bidding_proposal_model.items(i);

        key.pid = 0;
        if (item.has_pos_id() && !item.pos_id().empty())
        {
            key.pid = atoll(item.pos_id().c_str());
            if (key.pid == 0)
            {
                key.pid = util::Func::BytesHash64(item.pos_id().data(), item.pos_id().size());
            }
        }
        std::string value = item.SerializeAsString();
        
        this->SetMemkv((const char*)&key, sizeof(BiddingProposalKey), value.data(), value.size());
    }

    LOG_INFO("Update OK!file items=%d", bidding_proposal_model.items_size());
    return true;
}


} // namespace ors
} // namespace poseidon

