/**
**/

//include STD C/C++ head files
#include <fstream>

//include third_party_lib head files
#include "src/model_updater/model/base_param_handler.h"
#include "src/model_updater/api/structs.h"
#include "third_party/cityhash/include/city.h"
#include "util/log.h"
#include "util/proto_helper.h"
#include "util/func.h"


namespace poseidon
{
namespace model_updater
{

BaseParamHandler::BaseParamHandler()
{

}

BaseParamHandler::~BaseParamHandler()
{

}

bool BaseParamHandler::Update()
{  
    ors::AlgoModelData algo_model_data;
    if(!util::ParseProtoFromTextFormatFile(m_watch_file.c_str(), algo_model_data.mutable_base_param_model_data()))
    {
        LOG_ERROR("ParseProtoFromTextFormatFile %s Failed!", m_watch_file.c_str());
        return false;
    }

    BaseParamKey key;
    const ors::BaseParamModel& base_param_model = algo_model_data.base_param_model_data();
    for (int i = 0; i < base_param_model.adx_base_params_size(); i++)
    {
        const ors::AdxBaseParam& adx_base_param = base_param_model.adx_base_params(i);
        key.source = adx_base_param.adx_id();

        std::string val = adx_base_param.SerializeAsString();
        LOG_INFO("Update BaseParam source=%d, proto size=%d", 
                adx_base_param.adx_id(), val.size()); 
        this->SetMemkv((char*)&key, sizeof(BaseParamKey), val.data(), val.size());
    }

    for (int i = 0; i < base_param_model.pos_base_params_size(); i++)
    {
        const ors::PosBaseParam& pos_base_param = base_param_model.pos_base_params(i);
        key.source = pos_base_param.adx_id();
        key.pid = atoll(pos_base_param.pos_id().c_str());
        if (key.pid == 0) {
            key.pid = util::Func::BytesHash64(pos_base_param.pos_id().data(), pos_base_param.pos_id().size());
        }

        std::string val = pos_base_param.SerializeAsString();
        LOG_INFO("Update BaseParam source=%d, pid=%s, proto size=%d", 
                pos_base_param.adx_id(),pos_base_param.pos_id().c_str(), val.size()); 

        this->SetMemkv((char*)&key, sizeof(BaseParamKey), val.data(), val.size());
    }

    LOG_INFO("Update OK!file adx items=%d, pos_items=%d", 
            base_param_model.adx_base_params_size(), base_param_model.pos_base_params_size());
    return true;
}


} // namespace ors
} // namespace poseidon

