#include <string>
#include <iostream>
#include <locale>
#include <vector>
#include <algorithm>
#include <boost/locale/encoding_utf.hpp>
#include <boost/algorithm/string.hpp>
#include "src/model_updater/api/algo_model_data_api.h"
#include "src/model_updater/api/algo_model_data_dumper.h"

using namespace poseidon;
using namespace poseidon::model_updater;
using namespace std;
void usage() {
    cout << "./dump_model_data data_id" << endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        usage();
        return -1;
    }
    int data_id = atoi(argv[1]);
    if (data_id == 11) {
        AlgoModelDataDumper<BaseParamKey, poseidon::ors::AdxBaseParam> dumper;
        dumper.Init();
        dumper.DumpPB(data_id);
        return 0;
    }

    if (data_id == 12) {
        AlgoModelDataDumper<BudgetPacingKey, BudgetPacingValue> dumper;
        dumper.Init();
        dumper.Dump(data_id);
        return 0;
    }

    if (data_id == 21) {
        AlgoModelDataDumper<StatRateKey, StatRateValue> dumper;
        dumper.Init();
        dumper.Dump(data_id);
        return 0;
    }

    if (data_id == 14) {
        AlgoModelDataDumper<SpotGradeKey, SpotGradeValue> dumper;
        dumper.Init();
        dumper.Dump(data_id);
        return 0;
    }

    if (data_id == 15) {
        AlgoModelDataDumper<VideoContextGradeKey, VideoContextGradeValue> dumper;
        dumper.Init();
        dumper.Dump(data_id);
        return 0;
    }

    if (data_id == 16 || data_id == 17) {
        AlgoModelDataDumper<LRKey, LRValue> dumper;
        dumper.Init();
        dumper.Dump(data_id);
        return 0;
    }

    if (data_id == 20) {
        mem_sync::KVApi kv_api;
        kv_api.init();
        std::string key = "0";
        std::string val;
        int ret = kv_api.get(data_id, key, val);
        if (ret > 1) {
            cout << "No scoring params" << endl;
            return -1;
        } else if (ret < 0) {
            cout << "Get scoring params error" << endl;
            return -1;
        }
        scoring::ScoringParams params;
        params.ParseFromString(val);
        cout << params.DebugString() << endl;
        return 0;
    }

    if (data_id == 22) {
        AlgoModelDataDumper<BiddingProposalKey, ors::BiddingProposalItem> dumper;
        dumper.Init();
        dumper.DumpPB(data_id);
        return 0;
    }

    if (data_id == 23) {
        AlgoModelDataDumper<PayFactorKey, PayFactorValue> dumper;
        dumper.Init();
        dumper.Dump(data_id);
        return 0;
    }

    return 0;
}
