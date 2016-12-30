/**
 **/

//include STD C/C++ head files
#include <fstream>

//include third_party_lib head files
#include "src/model_updater/model/spot_grade_handler.h"
#include "src/model_updater/api/structs.h"
#include "third_party/cityhash/include/city.h"
#include "util/log.h"
#include "util/proto_helper.h"
#include "util/func.h"

namespace poseidon {
namespace model_updater {

SpotGradeHandler::SpotGradeHandler() {

}

SpotGradeHandler::~SpotGradeHandler() {

}

bool SpotGradeHandler::Update() {
    ors::AlgoModelData algo_model_data;
    if (!util::ParseProtoFromBinaryFormatFile(m_watch_file.c_str(),
            algo_model_data.mutable_spot_grade_model_data())) {
        LOG_ERROR("ParseProtoFromTextFormatFile %s Failed!",
                m_watch_file.c_str());
        return false;
    }

    SpotGradeKey key;
    SpotGradeValue value;
    const ors::SpotGradeModel& spot_grade_model = algo_model_data
            .spot_grade_model_data();
    for (int i = 0; i < spot_grade_model.items_size(); i++) {
        const ors::SpotGradeItem& item = spot_grade_model.items(i);
        key.source = item.adx_id();
        if (item.has_pos_id()) {
            key.pid = atoll(item.pos_id().c_str());
            if (key.pid == 0) {
                key.pid = util::Func::BytesHash64(item.pos_id().data(),
                        item.pos_id().size());
            }
        }
        key.app_id = atoll(item.app_name().c_str());
        if (0 == key.app_id) {
            key.app_id = util::Func::BytesHash64(item.app_name().data(),
                    item.app_name().size());
        }

        value.quality = item.quality();
        value.grade = item.grade();

        this->SetMemkv((const char*) &key, sizeof(SpotGradeKey),
                (const char*) &value, sizeof(SpotGradeValue));
    }

    LOG_INFO("Update OK!file items=%d", spot_grade_model.items_size());
    return true;
}

} // namespace ors
} // namespace poseidon

