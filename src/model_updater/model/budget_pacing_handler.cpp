/**
**/

//include STD C/C++ head files
#include <fstream>

//include third_party_lib head files
#include "src/model_updater/model/budget_pacing_handler.h"
#include "src/model_updater/api/structs.h"
#include "third_party/cityhash/include/city.h"
#include "util/log.h"
#include "util/proto_helper.h"
#include "util/func.h"


namespace poseidon
{
namespace model_updater
{

BudgetPacingHandler::BudgetPacingHandler()
{

}

BudgetPacingHandler::~BudgetPacingHandler()
{

}

bool BudgetPacingHandler::Update()
{  
    ors::AlgoModelData algo_model_data;
    if(!util::ParseProtoFromBinaryFormatFile(m_watch_file.c_str(), algo_model_data.mutable_budget_pacing_model_data()))
    {
        LOG_ERROR("ParseProtoFromTextFormatFile %s Failed!", m_watch_file.c_str());
        return false;
    }

    BudgetPacingKey key;
    BudgetPacingValue value;
    const ors::BudgetPacingModel& budget_pacing_model = algo_model_data.budget_pacing_model_data();
    for (int i = 0; i < budget_pacing_model.items_size(); i++)
    {
        const ors::BudgetPacingItem& item = budget_pacing_model.items(i);
        key.source = item.adx_id();
        key.campaign_id = item.campaign_id();

        value.budget_pacing_ratio = item.budget_pacing_ratio();
        value.budget_exceeding_ratio = item.budget_exceeding_ratio();
        value.bid_mode = item.bidding_mode();
        value.fixed_price = item.fixed_price();

        this->SetMemkv((const char*)&key, sizeof(BudgetPacingKey), (const char*)&value, sizeof(BudgetPacingValue));
    }

    LOG_INFO("Update OK!file items=%d", budget_pacing_model.items_size());
    return true;
}


} // namespace ors
} // namespace poseidon

