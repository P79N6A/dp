#ifndef _ORS_OFFLINE_CXR_STATS_STAT_HIERARCHY_
#define _ORS_OFFLINE_CXR_STATS_STAT_HIERARCHY_

#include <vector>
#include "protocol/src/poseidon_proto.h"
#include "stat_layer.h"

namespace poseidon {
namespace ors_offline {

enum StatRateLevel {
    StatRateSource = 0, StatRateOs, StatRatePos, StatRateView, StatRateCampaign, StatRateAd, StatRateCreative, StatRateMaxLevel
};

class StatHierarchy {
public:
    ~StatHierarchy();
    void Init(const CXRStatsConfig_ModelParams &config);
    void Clone(StatHierarchy &other);

    std::vector<StatLayer*>* GetLayerList() {
        return &_layer_list;
    }

    int ReadBottomStatFromFile(const char* file_path);
    int ReadStatFromFile(const char* file_path);
    int SaveEStatFile(const char* file_path);
    void GroupUp();
    void EAdd(StatHierarchy* other);
    void CalERates();
    void CalCpx();
private:
    CXRStatsConfig_ModelParams _config;

    std::vector<StatLayer*> _layer_list;

};

}
}
#endif
