#include "query_context.h"
#include <algorithm>
#include "util/func.h"
#include "src/model_updater/api/structs.h"
#include "src/model_updater/api/algo_model_data_api.h"
#include "src/monitor/api/monitor_api.h"
#include <iostream>

namespace poseidon {
namespace scoring {

QueryContext::QueryContext() {
    _req = NULL;
    _video_info = NULL;
}

QueryContext::~QueryContext() {
}

int QueryContext::Init() {
    return 0;
}

int QueryContext::Fini() {
    return 0;
}

int QueryContext::ResetContext(const ScoringRequest & req) {
    _req = &req;
    if (RequestPosNum() <= 0) {
        LOG_ERROR("No pos in scoring request");
        return -1;
    }
    _base_video_info.ResetInfo(Source(), PrimaryPosInfo().video());
    _video_info = &_base_video_info;
    return 0;
}

bool QueryContext::VideoGrade(float &video_grade) {
    using namespace std;
    using namespace model_updater;
    if (!_video_info->IsValid()) {
        LOG_DEBUG("not video");
        return false;
    }

    VideoContextGradeValue* value = NULL;
    VideoContextGradeKey key;
    key.source = Source();
    key.fchannel = _video_info->FChannel();

    // 视频ID
    if (_video_info->HasVId()) {
        key.context = _video_info->VId();
        key.context_type = VIDEO_CONTEXT_TYPE_VID;
        if (AlgoModelDataApi::get_mutable_instance().GetVideoContextGradeValue(
                key, &value)) {
            video_grade = value->quality;
            LOG_DEBUG("scoring vid quality: %d %f", key.context,
                    value->quality);
            if (ADX_ID_YOUKU == key.source) {
                MON_ADD(ATTR_SVR_SCORING_YOUKU_USE_VID, 1);
            } else if (ADX_ID_IQIYI == key.source) {
                MON_ADD(ATTR_SVR_SCORING_IQIYI_USE_VID, 1);
            }
            return true;
        }
        LOG_DEBUG("Fail to get vid quality: %s", key.to_string());
    }

    // 节目ID
    if (_video_info->HasShowId()) {
        key.context = _video_info->ShowId();
        key.context_type = VIDEO_CONTEXT_TYPE_SHOW_ID;
        if (AlgoModelDataApi::get_mutable_instance().GetVideoContextGradeValue(
                key, &value)) {
            video_grade = value->quality;
            LOG_DEBUG("scoring showid quality: %f", key.context, video_grade);
            if (ADX_ID_YOUKU == key.source) {
                MON_ADD(ATTR_SVR_SCORING_YOUKU_USE_SHOWID, 1);
            } else if (ADX_ID_IQIYI == key.source) {
                MON_ADD(ATTR_SVR_SCORING_IQIYI_USE_SHOWID, 1);
            }
            return true;
        }
        LOG_DEBUG("Fail to get showid quality: %s", key.to_string());
    }

    // 专辑ID
    // todo

    // title
    if (Source() != ADX_ID_YOUKU && _video_info->HasTitle()) {
        float max_quality = -1.0f;
        for (size_t i = 0; i < _video_info->TitleSegments().size(); i++) {
            key.context = _video_info->TitleSegments()[i];
            key.context_type = VIDEO_CONTEXT_TYPE_TITLE;
            if (AlgoModelDataApi::get_mutable_instance()
                    .GetVideoContextGradeValue(key, &value)) {
                max_quality = max(max_quality, value->quality);
                if (ADX_ID_YOUKU == key.source) {
                    MON_ADD(ATTR_SVR_SCORING_YOUKU_USE_TITLE, 1);
                } else if (ADX_ID_IQIYI == key.source) {
                    MON_ADD(ATTR_SVR_SCORING_IQIYI_USE_TITLE, 1);
                }
            }
        }
        if (max_quality >= 0) {
            video_grade = value->quality;
            LOG_DEBUG("scoring title quality: %f", key.context, video_grade);
            return true;
        }
        LOG_DEBUG("Fail to get title quality: %s", key.to_string());
    }

    // 关键词
    if (_video_info->HasKeyWord()) {
        float max_quality = -1.0f;
        for (size_t i = 0; i < _video_info->Keywords().size(); i++) {
            key.context = _video_info->Keywords()[i];
            key.context_type = VIDEO_CONTEXT_TYPE_KEYWORD;
            if (AlgoModelDataApi::get_mutable_instance()
                    .GetVideoContextGradeValue(key, &value)) {
                max_quality = max(max_quality, value->quality);
                if (ADX_ID_YOUKU == key.source) {
                    MON_ADD(ATTR_SVR_SCORING_YOUKU_USE_KEYWORD, 1);
                } else if (ADX_ID_IQIYI == key.source) {
                    MON_ADD(ATTR_SVR_SCORING_IQIYI_USE_KEYWORD, 1);
                }
            }
        }
        if (max_quality >= 0) {
            video_grade = value->quality;
            LOG_DEBUG("scoring keyword quality: %f", key.context, video_grade);
            return true;
        }
        LOG_DEBUG("Fail to get keyword quality: %s", key.to_string());
    }

    // 二级分类
    if (_video_info->hasSChannel()) {
        float max_quality = -1.0f;
        for (size_t i = 0; i < _video_info->SChannels().size(); i++) {
            key.context = _video_info->SChannels()[i];
            key.context_type = VIDEO_CONTEXT_TYPE_SCHANNEL;
            if (AlgoModelDataApi::get_mutable_instance()
                    .GetVideoContextGradeValue(key, &value)) {
                max_quality = max(max_quality, value->quality);
            }
        }

        if (max_quality > 0) {
            video_grade = value->quality;
            LOG_DEBUG("scoring schannel quality: %f", key.context, video_grade);
            return true;
        }
        LOG_DEBUG("Fail to get schannel quality: %s", key.to_string());
    }

    // 一级分类
    if (_video_info->HasFChannel()) {
        key.context_type = VIDEO_CONTEXT_TYPE_FCHANNEL;
        key.context = _video_info->FChannel();
        if (AlgoModelDataApi::get_mutable_instance().GetVideoContextGradeValue(
                key, &value)) {
            video_grade = value->quality;
            LOG_DEBUG("scoring fchannel quality: %f", key.context, video_grade);
            return true;
        }
        LOG_DEBUG("Fail to get fchannel quality: %s", key.to_string());
    }

    return false;
}

bool QueryContext::SpotGrade(float &spot_grade) {
    using namespace model_updater;
    SpotGradeKey key;
    SpotGradeValue* value = NULL;

    key.source = Source();
    key.pid = PrimaryPid();
    key.app_id = AppId();

    if (AlgoModelDataApi::get_mutable_instance().GetSpotGradeValue(key,
            &value)) {
        spot_grade = value->quality;
        return true;
    }

    return false;
}

bool QueryContext::UserGrade(int user_grade_plan_id, int &user_grade) {

    for (int i = 0; i < PrimaryPosInfo().targetting_size(); i++) {
        const common::Targetting& targetting = PrimaryPosInfo().targetting(i);
        if ((int) targetting.type() == user_grade_plan_id
                && targetting.value_size() > 0) {
            //only one value
            user_grade = util::Func::to_int(targetting.value(0));
            return true;
        }
    }

    return false;
}

bool QueryContext::SeedUserGrade(int &seed_user_grade,
        int &seed_active_user_grade) {

    int seed = -1;
    for (int i = 0; i < PrimaryPosInfo().targetting_size(); i++) {
        const common::Targetting& targetting = PrimaryPosInfo().targetting(i);
        if (targetting.type() == USER_TAG_TYPE_SEED_USER
                && targetting.value_size() >= 2) {

            for (int j = 1; j < targetting.value_size(); j += 2) {
                seed = util::Func::to_int(targetting.value(j));
                if (SeedUserCode(seed) == 10) {
                    seed_user_grade = seed;
                } else if (SeedUserCode(seed)
                        > SeedUserCode(seed_active_user_grade)) {
                    seed_active_user_grade = seed;
                }
            }
            return true;
        }
    }
    return false;
}

}
}
