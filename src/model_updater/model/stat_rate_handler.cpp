/**
**/

//include STD C/C++ head files
#include <fstream>

//include third_party_lib head files
#include "src/model_updater/model/stat_rate_handler.h"
#include "src/model_updater/api/structs.h"
#include "third_party/cityhash/include/city.h"
#include "util/log.h"
#include "util/proto_helper.h"
#include "util/func.h"


namespace poseidon
{
namespace model_updater
{

StatRateHandler::StatRateHandler()
{

}

StatRateHandler::~StatRateHandler()
{

}

bool StatRateHandler::Update()
{
    ors::AlgoModelData algo_model_data;
    if(!util::ParseProtoFromBinaryFormatFile(m_watch_file.c_str(), algo_model_data.mutable_stat_rate_model_data()))
    {
        LOG_ERROR("ParseProtoFromTextFormatFile %s Failed!", m_watch_file.c_str());
        return false;
    }

    StatRateKey key;
    StatRateValue value;
    const ors::StatRateModel& stat_rate_model = algo_model_data.stat_rate_model_data();
    for (int i = 0; i < stat_rate_model.items_size(); i++)
    {
        const ors::StatRateItem& item = stat_rate_model.items(i);
        key.source = item.source_id();
        key.os_type = item.os_id();
        key.pid = 0;
        if (item.has_pos_id() && !item.pos_id().empty())
        {
            key.pid = atoll(item.pos_id().c_str());
            if (key.pid == 0)
            {
                key.pid = util::Func::BytesHash64(item.pos_id().data(), item.pos_id().size());
            }
        }
        key.view_type = item.view_type_id();
        key.advertiser_id = item.advertiser_id();
        key.campaign_id = item.campaign_id();
        key.ad_id = item.ad_id();
        key.creative_id = item.creative_id();

        value.ctr = item.ctr();
        value.cvr = item.cvr();
        value.cpm = item.cpm();
        value.cpc = item.cpc();
        value.cpa = item.cpa();
        value.imprs = item.impressions();
        value.costs = item.costs();
        value.clicks = item.clicks();
        value.binds = item.binds();

        this->SetMemkv((const char*)&key, sizeof(StatRateKey), (const char*)&value, sizeof(StatRateValue));
    }

    LOG_INFO("Update OK!file items=%d", stat_rate_model.items_size());
    return true;
}


} // namespace ors
} // namespace poseidon

