/**
 **/

#ifndef _MODEL_UPDATER_API_ALGO_MODEL_DATA_API_H_
#define _MODEL_UPDATER_API_ALGO_MODEL_DATA_API_H_
//include STD C/C++ head files

//include third_party_lib head files
#include "src/model_updater/api/structs.h"
#include "third_party/google/protobuf/text_format.h"
#include "third_party/boost/include/boost/serialization/singleton.hpp"
#include "protocol/src/poseidon_proto.h"
#include "util/func.h"
#include "util/log.h"
#include "src/mem_sync/data_api/kv_api.h"

namespace poseidon {
namespace model_updater {

#define ALGO_MODEL_DATA_ID_BASE_PARAM 11
#define ALGO_MODEL_DATA_ID_BUDGET_PACING 12
#define ALGO_MODEL_DATA_ID_SPOT_GRADE 14
#define ALGO_MODEL_DATA_ID_VIDEO_CONTEXT_GRADE 15
#define ALGO_MODEL_DATA_ID_SCORING_PARAM 20
#define ALGO_MODEL_DATA_ID_STAT_RATE 21
#define ALGO_MODEL_DATA_ID_BIDDING_PROPOSAL 22
#define ALGO_MODEL_DATA_ID_PAY_FACTOR 23

class AlgoModelDataApi: public boost::serialization::singleton<AlgoModelDataApi> {

public:
    AlgoModelDataApi();
    ~AlgoModelDataApi();

    bool Init();
    void Fini();

    bool GetPayFactorValue(const PayFactorKey& key, PayFactorValue** value) {
        size_t val_size = 0;
        char* val = NULL;
        if (0 == m_kv_api.get(ALGO_MODEL_DATA_ID_PAY_FACTOR,
                sizeof(PayFactorKey), (const char*) &key, val_size, val)) {
            *value = (PayFactorValue*) val;
            return true;
        }
        return false;
    }

    bool GetBiddingProposalValue(const BiddingProposalKey& key, ors::BiddingProposalItem* value) {
        size_t val_size = 0;
        char* val = NULL;

        if (0 == m_kv_api.get(ALGO_MODEL_DATA_ID_BIDDING_PROPOSAL,
               sizeof(BiddingProposalKey), (const char*) &key, val_size, val)) {
            value->ParseFromArray(val, val_size);
            return true;
        }

        return false;
    }
        
    bool GetStatRateValue(const StatRateKey& key, StatRateValue** value) {
        size_t val_size = 0;
        char* val = NULL;
        if (0 == m_kv_api.get(ALGO_MODEL_DATA_ID_STAT_RATE,
                sizeof(StatRateKey), (const char*) &key, val_size, val)) {
            *value = (StatRateValue*) val;
            return true;
        }
        return false;
    }

    bool GetVideoContextGradeValue(const VideoContextGradeKey& key,
            VideoContextGradeValue** value) {
        size_t val_size = 0;
        char* val = NULL;
        if (0 == m_kv_api.get(ALGO_MODEL_DATA_ID_VIDEO_CONTEXT_GRADE,
                sizeof(VideoContextGradeKey), (const char*) &key, val_size,
                val)) {
            *value = (VideoContextGradeValue*) (val);
            return true;
        }
        return false;
    }

    bool GetVideoContextGradeValue(int data_id, const VideoContextGradeKey& key,
            VideoContextGradeValue** value) {
        size_t val_size = 0;
        char* val = NULL;
        if (0 == m_kv_api.get(data_id,
                sizeof(VideoContextGradeKey), (const char*) &key, val_size,
                val)) {
            *value = (VideoContextGradeValue*) (val);
            return true;
        }
        return false;
    }

    bool GetBudgetPacingValue(const BudgetPacingKey& key,
            BudgetPacingValue** value) {
        size_t val_size = 0;
        char* val = NULL;
        if (0 == m_kv_api.get(ALGO_MODEL_DATA_ID_BUDGET_PACING,
                sizeof(BudgetPacingKey), (const char*) &key, val_size, val)) {
            *value = (BudgetPacingValue*) val;
            return true;
        }
        return false;
    }

    bool GetSpotGradeValue(const SpotGradeKey& key, SpotGradeValue** value) {
        size_t val_size = 0;
        char* val = NULL;
        if (0 == m_kv_api.get(ALGO_MODEL_DATA_ID_SPOT_GRADE,
                sizeof(SpotGradeKey), (const char*) &key, val_size, val)) {
            *value = (SpotGradeValue*) val;
            return true;
        }
        return false;
    }

    bool GetBaseParamValue(uint32_t source, ors::AdxBaseParam* param) {
        BaseParamKey key;
        key.source = source;

        size_t val_size = 0;
        char* val = NULL;

        if (0 == m_kv_api.get(ALGO_MODEL_DATA_ID_BASE_PARAM,
                sizeof(BaseParamKey), (const char*) &key, val_size, val)) {
            param->ParseFromArray(val, val_size);
            return true;
        }

        return false;
    }
    bool GetBaseParamValue(uint32_t source, uint64_t pid, ors::PosBaseParam* param) {
        BaseParamKey key;
        key.source = source;
        key.pid = pid;

        size_t val_size = 0;
        char* val = NULL;
         
        if (0 == m_kv_api.get(ALGO_MODEL_DATA_ID_BASE_PARAM,
                        sizeof(BaseParamKey), (const char*) &key, val_size,
                        val)) {
            param->ParseFromArray(val, val_size);
            return true;
        }

        return false;
    }


    bool GetLRModelMeta(uint32_t data_id, ors::LRModel::Meta* meta) {
        std::map<int,int>::iterator iter = m_data_id_version.find(data_id);
        int cur_ver = m_kv_api.get_version(data_id);
        if (iter != m_data_id_version.end() && iter->second  == cur_ver)
        {
            return true;
        }
        m_data_id_version[data_id] = cur_ver;

        size_t val_size = 0;
        char* val = NULL;

        std::string key = "meta";
        if (0 == m_kv_api.get(data_id, key.size(), key.data(), val_size, val)) {
            meta->ParseFromArray(val, val_size);
            return true;
        }

        return false;
    }

    bool GetLRModelValue(uint32_t data_id, const LRKey& key, LRValue** value) {
        size_t val_size = 0;
        char* val = NULL;
        if (0 == m_kv_api.get(data_id, sizeof(LRKey), (const char*) &key,
                val_size, val)) {
            *value = (LRValue*) val;
            return true;
        }
        return false;
    }

    bool GetScoringParams(scoring::ScoringParams *params) {
        static int last_version = -1;
        int dataid = ALGO_MODEL_DATA_ID_SCORING_PARAM;
        int version = m_kv_api.get_version(dataid);
        if (version == last_version) {
            LOG_DEBUG("data not update");
            return false;
        } else {
            last_version = version;
        }

        std::string key = "0";
        std::string val;
        int ret = m_kv_api.get(dataid, key, val);
        if (ret > 1) {
            LOG_DEBUG("No key %s", key.c_str());
            return false;
        } else if (ret < 0) {
            LOG_ERROR("Get key %s error", key.c_str());
            return false;
        }
        params->ParseFromString(val);
        LOG_DEBUG("get key %s", key.c_str());
        return true;
    }




private:
    mem_sync::KVApi m_kv_api;
    std::map<int, int> m_data_id_version;
};

} // namespace model_updater
} // namespace poseidon

#endif // _MODEL_UPDATER_API_ALGO_MODEL_DATA_API_H_

