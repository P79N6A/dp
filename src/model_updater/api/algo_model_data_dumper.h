/**
 **/

#ifndef _MODEL_UPDATER_API_ALGO_MODEL_DATA_DUMPER_H_
#define _MODEL_UPDATER_API_ALGO_MODEL_DATA_DUMPER_H_
//include STD C/C++ head files

//include third_party_lib head files
#include "src/model_updater/api/structs.h"
#include "third_party/google/protobuf/text_format.h"
#include "third_party/boost/include/boost/serialization/singleton.hpp"
#include "protocol/src/poseidon_proto.h"
#include "util/func.h"
#include "src/mem_sync/data_api/kv_api.h"

namespace poseidon {
namespace model_updater {
template<class KEY, class VALUE>
class AlgoModelDataDumper {
public:
    AlgoModelDataDumper()
    {
    }
    ~AlgoModelDataDumper()
    {
    }

    bool Init()
    {
        return m_kv_api.init();
    }

    void Dump(int data_id)
    {
        size_t ks, vs;
        char *k, *v;
        KEY* key;
        VALUE* value;
        mem_sync::KVIter *iter = m_kv_api.get_iter(data_id);
        while (iter && 0 == iter->next(ks, k, vs, v)) {
            key = (KEY*)k;
            value = (VALUE*)v;
            std::cout << key->to_string() << "," << value->to_string() << std::endl;
        }
    }

    void DumpPB(int data_id)
    {
        size_t ks, vs;
        char *k, *v;
        KEY* key;
        VALUE value;
        mem_sync::KVIter *iter = m_kv_api.get_iter(data_id);
        while (iter && 0 == iter->next(ks, k, vs, v)) {
            key = (KEY*)k;
            if (value.ParseFromArray(v, vs)) {
                std::cout << key->to_string() << std::endl;
                value.PrintDebugString();
            }
        }
    }

private:
    mem_sync::KVApi m_kv_api;
};

} // namespace model_updater
} // namespace poseidon

#endif // _MODEL_UPDATER_API_ALGO_MODEL_DATA_DUMPER_H_

