#include "scoring_param_handler.h"
#include "protocol/src/poseidon_proto.h"
#include "util/log.h"
#include "util/proto_helper.h"
#include "util/func.h"
#include "src/model_updater/api/structs.h"

namespace poseidon {
namespace model_updater {

ScoringParamHandler::ScoringParamHandler() {

}

ScoringParamHandler::~ScoringParamHandler() {

}

bool ScoringParamHandler::Update() {
    using namespace std;
    scoring::ScoringParams scoring_params;
    if (!util::ParseProtoFromTextFormatFile(m_watch_file.c_str(),
            &scoring_params)) {
        LOG_ERROR("ParseProtoFromTextFormatFile %s Failed!",
                m_watch_file.c_str());
        return false;
    }

    string key = "0";
    string val = scoring_params.SerializeAsString();
    if (!this->SetMemkv(key, val)) {
        LOG_ERROR("Fail to set scoring params");
    }
    LOG_INFO("Scoring params are set %s", key.c_str());

    return true;
}

}
}

