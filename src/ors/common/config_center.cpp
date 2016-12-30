/**
**/

//include STD C/C++ head files
#include <fstream>

//include third_party_lib head files
#include "src/ors/common/config_center.h"
#include "util/log.h"
#include "util/proto_helper.h"

namespace poseidon
{
namespace ors
{

ConfigCenter::ConfigCenter()
{
}

ConfigCenter::~ConfigCenter()
{

}

bool ConfigCenter::Init(const std::string& path)
{
    const std::string algo_model_conf = "algo_model.conf";
    std::string conf_path = "../conf/";
    if (!path.empty())
    {   
        conf_path = path;
    }

    std::string conf_file =  conf_path + algo_model_conf;
    if (!util::ParseProtoFromTextFormatFile(conf_file.c_str(), &m_algo_model_config))
    {
        LOG_ERROR("LoadConfig for algo_model Failed!");
        return false;
    }

    LOG_INFO("ConfigCenter Init OK!");
    return true;
}


} // namespace ors
} // namespace poseidon

