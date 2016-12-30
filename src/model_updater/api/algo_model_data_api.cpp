/**
**/

//include STD C/C++ head files
#include <fstream>

//include third_party_lib head files
#include "src/model_updater/api/algo_model_data_api.h"
#include "util/log.h"
#include "util/proto_helper.h"

namespace poseidon
{
namespace model_updater
{

AlgoModelDataApi::AlgoModelDataApi()
{

}

AlgoModelDataApi::~AlgoModelDataApi()
{

}


bool AlgoModelDataApi::Init()
{
    if (!m_kv_api.init())
    {
        LOG_ERROR("KvApi Init Failed!");
        return false;
    }

    LOG_INFO("AlgoModelDataApi Init OK!");
    return true;
}


void AlgoModelDataApi::Fini()
{

}

} // namespace model_updater
} // namespace poseidon

