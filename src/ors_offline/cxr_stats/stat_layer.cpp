#include "stat_layer.h"

namespace poseidon {
namespace ors_offline {

std::string StatLayer::UpperKey(const std::string &key) {

    size_t index = key.find_last_of(stat_delimiter);
    if (index == std::string::npos) {
        return "";
    }
    return key.substr(0, index);
}

std::string StatLayer::KeyId(const std::string &key) {
    std::string delimiter = "^";
    size_t index = key.find_last_of(delimiter);
    if (index == std::string::npos) {
        return key;
    }
    return key.substr(index + delimiter.size());
}

void StatLayer::EAdd(StatLayer* other) {
    for (boost::unordered_map<std::string, StatInfo>::iterator it = other
            ->_stat_info_map.begin(); it != other->_stat_info_map.end(); ++it) {
        std::string key = it->first;
        StatInfo* stat_info = GetStatInfo(key);
        StatInfo* new_stat_info = &(it->second);
        uint32_t sufficient_clicks = _config->sufficient_clicks();
        uint32_t sufficient_binds = _config->sufficient_binds();
        if ((stat_info->clicks > sufficient_clicks
                && new_stat_info->clicks > sufficient_clicks)
                || stat_info->clicks > sufficient_clicks * 3) {
            stat_info->EAdd(new_stat_info, _config->time_smoothing_factor());
        } else {
            stat_info->RAdd(new_stat_info);
        }

        if ((stat_info->binds > sufficient_binds
                && new_stat_info->binds > sufficient_binds)
                || stat_info->binds > sufficient_binds * 3) {
            stat_info->EAddCvr(new_stat_info, _config->time_smoothing_factor());
        } else {
            stat_info->RAddCvr(new_stat_info);
        }
    }
}

void StatLayer::GroupUpper(bool push_up) {
    if (NULL == _upper_layer) {
        return;
    }
    for (boost::unordered_map<std::string, StatInfo>::iterator it =
            _stat_info_map.begin(); it != _stat_info_map.end(); ++it) {
        std::string upper_key = UpperKey(it->first);
        StatInfo* upper_stat_info = _upper_layer->GetStatInfo(upper_key);
        upper_stat_info->RAdd(&(it->second));
        upper_stat_info->RAddCvr(&(it->second));
    }
    if (push_up) {
        _upper_layer->GroupUpper(push_up);
    }
}

void StatLayer::ProcessUnderERate() {
    if (_under_layers.empty()) {
        return;
    }
    for (size_t i = 0; i < _under_layers.size(); ++i) {
        _under_layers[i]->CalERate();
        _under_layers[i]->ProcessUnderERate();
    }
}

void StatLayer::CalRRate() {
    for (boost::unordered_map<std::string, StatInfo>::iterator it =
            _stat_info_map.begin(); it != _stat_info_map.end(); ++it) {
        it->second.CalRRate(_config->ctr_default(), _config->cvr_default(),
                _config->cpa_default());
    }
}

void StatLayer::CalERate() {
    for (boost::unordered_map<std::string, StatInfo>::iterator it =
            _stat_info_map.begin(); it != _stat_info_map.end(); ++it) {
        std::string upper_key = UpperKey(it->first);
        StatInfo* upper_stat_info = _upper_layer->GetStatInfo(upper_key);
        it->second.CalERate(upper_stat_info, _config->ctr_bayes_factor(),
                _config->cvr_bayes_factor(), _config->cpa_bayes_factor());
    }
}

void StatLayer::CalCpx() {
    for (boost::unordered_map<std::string, StatInfo>::iterator it =
            _stat_info_map.begin(); it != _stat_info_map.end(); ++it) {
        it->second.CalCpx();
    }
}

}
}
