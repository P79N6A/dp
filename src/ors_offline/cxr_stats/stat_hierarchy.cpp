#include "stat_hierarchy.h"
#include <cstdio>
#include <ctime>
#include <fstream>
#include <boost/unordered_map.hpp>
#include "protocol/src/poseidon_proto.h"
#include "util/proto_helper.h"
#include "util/log.h"
#include "util/func.h"
#include "src/ors_offline/common/utility.h"

namespace poseidon {
namespace ors_offline {

StatHierarchy::~StatHierarchy() {
    for (size_t i = 0; i < _layer_list.size(); ++i) {
        delete _layer_list[i];
    }
}

void StatHierarchy::Init(const CXRStatsConfig_ModelParams &config) {

    _config = config;
    _layer_list.resize(StatRateMaxLevel);

    for (size_t i = 0; i < _layer_list.size(); ++i) {
        _layer_list[i] = new StatLayer(i, &_config);
    }

    _layer_list[StatRateSource]->AddUnderLayer(_layer_list[StatRateOs]);
    _layer_list[StatRateOs]->AddUnderLayer(_layer_list[StatRatePos]);
    _layer_list[StatRatePos]->AddUnderLayer(_layer_list[StatRateView]);
    _layer_list[StatRateView]->AddUnderLayer(_layer_list[StatRateCampaign]);
    _layer_list[StatRateCampaign]->AddUnderLayer(_layer_list[StatRateAd]);
    _layer_list[StatRateAd]->AddUnderLayer(_layer_list[StatRateCreative]);

}

void StatHierarchy::Clone(StatHierarchy &other) {
    for (size_t i = 0; i < _layer_list.size(); ++i) {
        _layer_list[i]->Clone(*(other._layer_list[i]));
    }
}

int StatHierarchy::ReadBottomStatFromFile(const char* file_path) {
    using namespace std;

    string ymd = getYmd(time(0));
    int update_time = util::Func::to_int(ymd);
    StatLayer* bottom = _layer_list[StatRateCreative];

    ifstream fin(file_path, ifstream::in);
    if (!fin.good()) {
        LOG_ERROR("Fail to load bottom stat from file: %s", file_path);
        return -1;
    }

    string line;
    while (std::getline(fin, line)) {
        StatInfo stat_info;

        size_t index = 0, pos = 0, count = 0;
        while (count < StatRateMaxLevel && string::npos != index) {
            index = line.find(stat_delimiter, pos);
            pos = index + stat_delimiter.size();
            count++;
        }
        if (count < StatRateMaxLevel) {
            LOG_ERROR("Incorrect stat file format");
            fin.close();
            return -1;
        }

        string key = line.substr(0, index);
        vector<string> id_list = split(key, stat_delimiter);
        bool has_zero_id = false;
        for (int i = 0; i< id_list.size(); ++i) {
            if (id_list[i] == "0") {
                has_zero_id = true;
                break;
            }
        }
        if (has_zero_id) {
            continue;
        }
        sscanf(line.substr(index).c_str(), "`%f`%f`%f`%f", &stat_info.imprs,
                &stat_info.costs, &stat_info.clicks, &stat_info.binds);
        stat_info.costs_cvr = stat_info.costs;
        stat_info.clicks_cvr = stat_info.clicks;
        stat_info.update_time = update_time;
        bottom->SetStatInfo(key, stat_info);
    }

    fin.close();
    return 0;
}

void StatHierarchy::GroupUp() {
    StatLayer* bottom = _layer_list[StatRateCreative];
    bottom->GroupUpper(true);
}

void StatHierarchy::EAdd(StatHierarchy* other) {
    for (size_t i = 0; i < _layer_list.size(); ++i) {
        _layer_list[i]->EAdd(other->_layer_list[i]);
    }
}

void StatHierarchy::CalERates() {
    _layer_list[StatRateSource]->CalRRate();
    _layer_list[StatRateSource]->ProcessUnderERate();
}

void StatHierarchy::CalCpx() {
    for (size_t i = 0; i < _layer_list.size(); ++i) {
        _layer_list[i]->CalCpx();
    }
}

int StatHierarchy::ReadStatFromFile(const char* file_path) {
    using namespace std;
    ifstream fin(file_path, ifstream::in);
    if (!fin.good()) {
        LOG_ERROR("Fail to load estat from file: %s", file_path);
        return -1;
    }

    string line;
    while (std::getline(fin, line)) {
        int level;
        char ckey[100], str[100];
        sscanf(line.data(), "%d\t%s\t%s\t%*s", &level, ckey, str);
        string key(ckey);
        _layer_list[level]->GetStatInfo(key)->DeserializeEStatFromString(str);
    }
    fin.close();
    return 0;
}

int StatHierarchy::SaveEStatFile(const char* file_path) {
    using namespace std;
    FILE* fout = fopen(file_path, "w");
    if (NULL == fout) {
        LOG_ERROR("Fail to save stat to file: %s", file_path);
        return -1;
    }

    for (int i = 0; i < _layer_list.size(); ++i) {
        boost::unordered_map<std::string, StatInfo>::iterator it =
                _layer_list[i]->StatInfoMap()->begin();
        while (it != _layer_list[i]->StatInfoMap()->end()) {
            fprintf(fout, "%d\t%s\t%s\n", i, it->first.c_str(),
                    it->second.SerializeEStatAsString().c_str());
            it++;
        }
    }
    fclose(fout);
    return 0;
}

}
}
