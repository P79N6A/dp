#ifndef SRC_SCORING_CONTEXT_QUERY_CONTEXT_H_
#define SRC_SCORING_CONTEXT_QUERY_CONTEXT_H_

#include <map>
#include <vector>
#include "src/scoring/common/common.h"
#include "video_info.h"

namespace poseidon {
namespace scoring {

class QueryContext {
public:
    QueryContext();
    virtual ~QueryContext();
    virtual int Init();
    virtual int Fini();
    virtual int ResetContext(const ScoringRequest & req);

    int32_t Source() {
        return _req->traffic_source();
    }

    int32_t OsType() {
        return _req->device_info().os() == "ios" ?
                2 : (_req->device_info().os() == "android" ? 1 : 0);

    }

    uint64_t PrimaryPid() {
        return HashId(_req->adz_info(0).id());
    }

    const sn::AdzInfo& PrimaryPosInfo() {
        return _req->adz_info(0);
    }

    uint64_t AppId() {
        return HashId(_req->app_info().name());
    }

    size_t RequestAdNum() {
        return _req->ad_size();
    }

    size_t RequestPosNum() {
        return _req->adz_info_size();
    }

    void GetExpParams(std::map<int, int> &int_params,
            std::map<int, float> &float_params) {

        for (int i = 0; i < _req->exp_param_size(); ++i) {
            int key = _req->exp_param(i).param_id();
            if (_req->exp_param(i).has_int_value()) {
                int_params[key] = _req->exp_param(i).int_value();
            } else if (_req->exp_param(i).has_float_value()) {
                float_params[key] = _req->exp_param(i).float_value();
            }
        }
    }

    bool VideoGrade(float& video_grade);
    bool SpotGrade(float& spot_grade);
    bool UserGrade(int user_grade_plan_id, int& user_grade);
    bool SeedUserGrade(int &seed_user_grade, int &seed_active_user_grade);

private:
    const ScoringRequest* _req;
    VideoInfo* _video_info;
    VideoInfo _base_video_info;

};

}
}

#endif /* SRC_SCORING_QUERY_CONTEXT_H_ */
