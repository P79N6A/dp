/**
**/

//include STD C/C++ head files
#include <fstream>

//include third_party_lib head files
#include "src/model_updater/model/pay_factor_handler.h"
#include "src/model_updater/api/structs.h"
#include "third_party/cityhash/include/city.h"
#include "util/log.h"
#include "util/proto_helper.h"
#include "util/func.h"


namespace poseidon
{
namespace model_updater
{

PayFactorHandler::PayFactorHandler()
{

}

PayFactorHandler::~PayFactorHandler()
{

}

bool PayFactorHandler::Update()
{
    ors::AlgoModelData algo_model_data;
    if(!util::ParseProtoFromBinaryFormatFile(m_watch_file.c_str(), algo_model_data.mutable_pay_factor_model_data()))
    {
        LOG_ERROR("ParseProtoFromTextFormatFile %s Failed!", m_watch_file.c_str());
        return false;
    }

    PayFactorKey key;
    PayFactorValue value;
    const ors::PayFactorModel& pay_factor_model = algo_model_data.pay_factor_model_data();
    for (int i = 0; i < pay_factor_model.items_size(); i++)
    {
        const ors::PayFactorItem& item = pay_factor_model.items(i);
        key.campaign_id = item.campaign_id();
        key.ad_id = item.ad_id();

        value.pay_factor = item.pay_factor();

        this->SetMemkv((const char*)&key, sizeof(PayFactorKey), (const char*)&value, sizeof(PayFactorValue));
    }

    LOG_INFO("Update OK!file items=%d", pay_factor_model.items_size());
    return true;
}


} // namespace ors
} // namespace poseidon

