/**
**/

#ifndef _ORS_CONFIG_CENTER_H_
#define _ORS_CONFIG_CENTER_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "third_party/boost/include/boost/serialization/singleton.hpp"
#include "protocol/src/poseidon_proto.h"

namespace poseidon
{
namespace ors
{

typedef ::google::protobuf::Message MyConfig;

class ConfigCenter : public boost::serialization::singleton<ConfigCenter>
{

public:
    ConfigCenter();
    virtual ~ConfigCenter();
    virtual bool Init(const std::string& path="");

    const model_updater::ModelUpdaterConfig& GetAlgoModelConfig()
    {
        return m_algo_model_config;
    }


protected:
    model_updater::ModelUpdaterConfig m_algo_model_config;

};
} // namespace ors
} // namespace poseidon

#endif // _ORS_CONFIG_CENTER_H_

