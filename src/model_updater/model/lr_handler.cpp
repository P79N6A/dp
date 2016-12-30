/**
**/

//include STD C/C++ head files
#include <fstream>

//include third_party_lib head files
#include "src/model_updater/model/lr_handler.h"
#include "src/model_updater/api/structs.h"
#include "third_party/cityhash/include/city.h"
#include "util/log.h"
#include "util/proto_helper.h"
#include "util/func.h"


namespace poseidon
{
namespace model_updater
{

LRHandler::LRHandler()
{

}

LRHandler::~LRHandler()
{

}

bool LRHandler::Update()
{  
    ors::AlgoModelData algo_model_data;
    if(!util::ParseProtoFromBinaryFormatFile(m_watch_file.c_str(), algo_model_data.mutable_lr_model_data()))
    {
        LOG_ERROR("ParseProtoFromTextFormatFile %s Failed!", m_watch_file.c_str());
        return false;
    }

    LRKey key;
    LRValue value;
    const ors::LRModel& lr_model = algo_model_data.lr_model_data();
    for (int i = 0; i < lr_model.items_size(); i++)
    {
        key.fea_hash = lr_model.items(i).fea_hash();
        value.weight = lr_model.items(i).weight();
        this->SetMemkv((const char*)&key, sizeof(LRKey), (const char*)&value, sizeof(LRValue));
    }

    std::string meta = lr_model.meta().SerializeAsString();
    this->SetMemkv("meta", meta);

    LOG_INFO("Update OK!file items=%d, meta size=%d", lr_model.items_size(), meta.size());
    return true;
}


} // namespace ors
} // namespace poseidon

