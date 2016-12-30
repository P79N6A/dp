#include "params.h"
#include "util/func.h"
#include "util/log.h"
#include "src/model_updater/api/algo_model_data_api.h"


namespace poseidon {

namespace scoring {

uint64_t posKey(const int32_t& source, const std::string& pid) {
    char key_char[100];
    int size = sprintf(key_char, "%d_%s", source, pid.c_str());
    return util::Func::BytesHash64(key_char, size);
}

int Params::Init() {
    return !model_updater::AlgoModelDataApi::get_mutable_instance().Init();
}


void Params::UpdateParams() {
    ScoringParams params;
    if (!model_updater::AlgoModelDataApi::get_mutable_instance()
            .GetScoringParams(&params)) {
        return;
    }

    for (int i = 0; i < params.adx_params_size(); ++i) {
        _adx_params_map[params.adx_params(i).source()] = params.adx_params(i);
        LOG_DEBUG("Load scoring adx params for source %d",
                params.adx_params(i).source());
    }
    for (int i = 0; i < params.pos_params_size(); ++i) {
        uint64_t key = posKey(params.pos_params(i).source(),
                params.pos_params(i).pid());
        _pos_params_map[key] = params.pos_params(i);
        LOG_DEBUG("Load scoring pos params for source %d, pid %s",
                params.pos_params(i).source(),
                params.pos_params(i).pid().c_str());
    }
    LOG_INFO("Scoring params updated");
}

AdxParams* Params::GetAdxParams(int32_t source) {
    if (_adx_params_map.find(source) == _adx_params_map.end()) {
        return NULL;
    }
    return &_adx_params_map[source];
}

PosParams* Params::GetPosParams(int32_t source, std::string pid) {
    uint64_t key = posKey(source, pid);
    if (_pos_params_map.find(key) == _pos_params_map.end()) {
        return NULL;
    }
    return &_pos_params_map[key];
}

}
}
