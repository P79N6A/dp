#ifndef SRC_ORS_OFFLINE_CXR_STATS_STAT_LAYER_H_
#define SRC_ORS_OFFLINE_CXR_STATS_STAT_LAYER_H_

#include <vector>
#include <boost/unordered_map.hpp>
#include "protocol/src/poseidon_proto.h"
#include "stat_info.h"

namespace poseidon {
namespace ors_offline {

class StatLayer {
public:

    StatLayer(uint32_t id, CXRStatsConfig_ModelParams* config) {
        _id = id;
        _config = config;
        _upper_layer = NULL;
    }

    void Clone(StatLayer &other) {
        _stat_info_map = other._stat_info_map;
    }

    //get & set
    uint32_t Id() {
        return _id;
    }
    void UpperLayer(StatLayer* upper) {
        _upper_layer = upper;
    }
    StatLayer* UpperLayer() {
        return _upper_layer;
    }
    void AddUnderLayer(StatLayer* under) {
        _under_layers.push_back(under);
        under->UpperLayer(this);
    }
    std::vector<StatLayer*>* UnderLayers() {
        return &_under_layers;
    }
    StatInfo* GetStatInfo(std::string &key) {
        return &_stat_info_map[key];
    }
    void SetStatInfo(std::string &key, StatInfo &stat_info) {
        _stat_info_map[key] = stat_info;
    }
    boost::unordered_map<std::string, StatInfo>* StatInfoMap() {
        return &_stat_info_map;
    }

    //process
    void EAdd(StatLayer* other);
    void GroupUpper(bool);
    void ProcessUnderERate();
    void CalRRate();
    void CalERate();
    void CalCpx();

    static std::string UpperKey(const std::string &key);
    static std::string KeyId(const std::string &key);

private:
    uint32_t _id;
    CXRStatsConfig_ModelParams* _config;
    boost::unordered_map<std::string, StatInfo> _stat_info_map;
    StatLayer* _upper_layer;
    std::vector<StatLayer*> _under_layers;
};

}
}

#endif /* SRC_ORS_OFFLINE_CXR_STATS2_STAT_LAYER_H_ */
