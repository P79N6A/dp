#include "scorer.h"
#include <algorithm>
#include <sstream>
#include "util/log.h"
#include "util/func.h"
#include "util/proto_helper.h"
#include "src/monitor/api/monitor_api.h"
#include "src/scoring/context/ad_info.h"
#include "src/scoring/context/params.h"
#include "src/scoring/common/common.h"

namespace poseidon {

namespace scoring {

Scorer::Scorer() {
    _cxr_model = NULL;
    _adx_params = NULL;
    _pos_params = NULL;
    _context_quality = -1;
    _seed_user_grade = -1;
    _seed_active_user_grade = -1;
    _user_grade = -1;
    _is_pdb = false;
}

Scorer::~Scorer() {
}

int Scorer::Init() {
    try {
        CheckError(_stat_rate_model.Init(),
                "Fail to init scoring stat rate model");
    } catch (int e) {
        return e;
    }
    return 0;
}

int Scorer::Fini() {
    try {
        CheckError(_stat_rate_model.Fini(),
                "Fail to fini scoring stat rate model");
    } catch (int e) {
        return e;
    }
    return 0;
}

int Scorer::Prepare(QueryContext* qcontext) {

    _seed_user_grade = -1;
    _seed_active_user_grade = -1;
    _user_grade = -1;
    _context_quality = -1;
    _is_pdb = false;

    int32_t source = qcontext->Source();
    _adx_params = Params::get_mutable_instance().GetAdxParams(source);
    if (NULL == _adx_params) {
        LOG_ERROR("Fail to get scoring adx params for source %d", source);
        return -1;
    }
    qcontext->GetExpParams(_exp_int_params, _exp_float_params);

    try {
        CheckError(_stat_rate_model.Prepare(qcontext),
                "Fail to prepare stat_rate model");
    } catch (int e) {
        return e;
    }
    _cxr_model = &_stat_rate_model;
    return 0;
}

void Scorer::calSeedUser(QueryContext* qcontext) {
    if (qcontext->SeedUserGrade(_seed_user_grade, _seed_active_user_grade)) {
        MON_ADD(ATTR_SVR_SCORING_HAS_SEED_USER_GRADE_COUNT, 1);
    }
}

void Scorer::calUserGrade(QueryContext* qcontext) {
    int user_grade_plan_id = _adx_params->user_grade_plan_id();
    int tmp = 0;
    if (getExpParams(EXP_PARAM_SCORING_USER_GRADE_PLAN_ID, tmp)) {
        user_grade_plan_id = tmp;
    }
    if (qcontext->UserGrade(user_grade_plan_id, tmp)) {
        _user_grade = tmp;
    } else {
        MON_ADD(ATTR_SVR_SCORING_NO_USER_GRADE_COUNT, 1);
    }
}

void Scorer::calContextGrade(QueryContext* qcontext) {
    float tmp = 0;
    if (qcontext->VideoGrade(tmp)) {
        _context_quality = tmp;
        return;
    } else {
        if (qcontext->Source() == ADX_ID_IQIYI || qcontext->Source() == ADX_ID_YOUKU) {
            MON_ADD(ATTR_SVR_SCORING_NO_VIDEO_GRADE_COUNT, 1);
        }
        if (qcontext->SpotGrade(tmp)) {
            _context_quality = tmp;
            return;
        }
    }
    MON_ADD(ATTR_SVR_SCORING_NO_CONTEXT_GRADE_COUNT, 1);
}

int Scorer::AnalyzeContext(QueryContext* qcontext) {
    calSeedUser(qcontext);
    calUserGrade(qcontext);
    calContextGrade(qcontext);
    LOG_DEBUG("Scoring seed user grade: %d", _seed_user_grade);
    LOG_DEBUG("Scoring seed active user grade: %d", _seed_active_user_grade);
    LOG_DEBUG("Scoring user grade: %d", _user_grade);
    LOG_DEBUG("Scoring context quality: %f", _context_quality);
    return 0;
}

bool Scorer::FilterRequest(QueryContext* qcontext, AdContext* ad_context) {

    if (qcontext->RequestAdNum() <= 0) {
        LOG_DEBUG("No ad in scoring request");
        MON_ADD(ATTR_SVR_SCORING_REQ_NO_AD_COUNT, 1);
        return false;
    }

    bool fbs = filterBySeedUser(qcontext);
    bool fbcg = filterByContextGrade(qcontext);
    bool fbug = filterByUserGrade(qcontext);
    bool fbp = filterByPdb(ad_context);

    if (fbs) {
        MON_ADD(ATTR_SVR_SCORING_FILTER_BY_USER_SEED, 1);
    }
    if (fbcg) {
        MON_ADD(ATTR_SVR_SCORING_FILTER_BY_CONTEXT, 1);
    }
    if (fbug) {
        MON_ADD(ATTR_SVR_SCORING_FILTER_BY_USER_GRADE, 1);
    }
    if (fbp) {
        MON_ADD(ATTR_SVR_SCORING_FILTER_BY_PDB, 1);
    }

    return fbs || fbcg || fbug || fbp;
}

bool Scorer::filterBySeedUser(QueryContext* qcontext) {
    int seed_user_grade_threshold = _adx_params->seed_user_threshold();
    LOG_DEBUG("seed_threshold: %d", seed_user_grade_threshold);
    LOG_DEBUG("seed_user_grade: %d\n", _seed_user_grade);
    int threshold = SeedUserCode(seed_user_grade_threshold);
    return (SeedUserCode(_seed_user_grade) >= threshold)
            || (SeedUserCode(_seed_active_user_grade) >= threshold);
}

bool Scorer::filterByUserGrade(QueryContext* qcontext) {
    if (_adx_params->disable_user_grade_filter()) {
        return true;
    }
    int user_grade_threshold = _adx_params->user_grade_threshold();
    LOG_DEBUG("user_threshold: %d", user_grade_threshold);
    LOG_DEBUG("user_grade: %d\n", _user_grade);
    int grade = (-1 == _user_grade) ? USER_GRADE_LOW_LIMIT : _user_grade;
    return grade <= user_grade_threshold;
}

bool Scorer::filterByContextGrade(QueryContext* qcontext) {
    if (_adx_params->disable_context_grade_filter()) {
        return true;
    }
    float context_grade_threshold = _adx_params->context_grade_threshold();
    LOG_DEBUG("context_threshold: %f", context_grade_threshold);
    return _context_quality >= context_grade_threshold;
}

bool Scorer::filterByPdb(AdContext* ad_context) {
    std::vector<AdInfo*>* ad_list = ad_context->GetAdList();
    for (size_t i = 0; i < ad_list->size(); ++i) {
        if (ad_list->at(i)->IsPdb()) {
            _is_pdb = true;
            return true;
        }
    }
    return false;
}

int Scorer::FilterAdByParam(AdContext* ad_context) {
    int size = _adx_params->exclude_ad_size();
    for (int i = 0; i < size; ++i) {
        AdInfo* ad_info = ad_context->GetAdInfo(_adx_params->exclude_ad(i));
        if (ad_info != NULL) {
            ad_info->Exclude();
        }
    }
    return 0;
}

int Scorer::IncludeAdByParam(AdContext* ad_context) {
    int size = _adx_params->include_ad_size();
    for (int i = 0; i < size; ++i) {
        AdInfo* ad_info = ad_context->GetAdInfo(_adx_params->include_ad(i));
        if (ad_info != NULL) {
            ad_info->Include();
        }
    }
    return 0;
}

int Scorer::ScoringAds(QueryContext* qcontext, AdContext* ad_context) {
    if (_is_pdb) {
        return 0;
    }

    std::vector<AdInfo*>* ad_list = ad_context->GetAdList();
    for (std::vector<AdInfo*>::iterator it = ad_list->begin();
            it != ad_list->end(); it++) {
        AdInfo* ad_info = *it;
        float ctr = _cxr_model->Ctr(ad_info);
        float cvr = _cxr_model->Cvr(ad_info);
        LOG_DEBUG("Scoring Ad %d:  ctr %f  cvr %f", ad_info->AdId(), ctr, cvr);
        float price = ad_info->CalPrice(ctr, cvr);
        ad_info->Score(ad_info->Score() * price * ctr * cvr);
    }
    return 0;
}

int Scorer::ChooseFinalAds(AdContext* ad_context, ScoringResponse &rsp) {

    if (_is_pdb || _adx_params->enable_random_choose_ad()) {
        ad_context->RandAndFilterAdList();
    } else {
        ad_context->SortAndFilterAdList();
    }
    std::vector<AdInfo*>* sort_list = ad_context->GetAdList();

    //ensure each campaign has one ad choosed
    std::set<uint32_t> campaign_set;

    for (size_t i = 0; i < sort_list->size(); ++i) {
        uint32_t campaign_id = sort_list->at(i)->CampaignId();
        if (campaign_set.find(campaign_id) == campaign_set.end()) {
            campaign_set.insert(campaign_id);
            sort_list->at(i)->Include();
        }
    }

    if (_is_pdb || _adx_params->enable_random_choose_ad()) {
        ad_context->RandAndFilterAdList();
    } else {
        ad_context->SortAndFilterAdList();
    }
    sort_list = ad_context->GetAdList();

    size_t rsp_ad_num = _adx_params->response_ad_num();

    rsp_ad_num = std::min(sort_list->size(), rsp_ad_num);
    for (size_t i = 0; i < rsp_ad_num; ++i) {
        AdInfo* ad_info = sort_list->at(i);
        const common::Ad* ad = ad_info->Ad();
        if (NULL == ad) {
            LOG_ERROR("Get null ad ptr in sorted ad list");
            return -1;
        }
        rsp.add_ad()->CopyFrom(*ad);
    }

    return 0;
}

int Scorer::FillOrsMsgResponse(ScoringResponse &rsp) {

    std::stringstream ss;

    if (_seed_active_user_grade >= 0) {
        rsp.mutable_scoring_to_ors_msg()->add_user_seed(
                _seed_active_user_grade);
        ss << _seed_active_user_grade;
    }
    if (_seed_user_grade >= 0) {
        rsp.mutable_scoring_to_ors_msg()->add_user_seed(_seed_user_grade);
        if (ss.tellp() > 0) {
            ss << "|";
        }
        ss << _seed_user_grade;
    }
    if (ss.tellp() > 0) {
        util::KeyValue* seed_kv = rsp.add_scoring_pvlogs();
        seed_kv->set_key("user_seed");
        seed_kv->set_value(ss.str());
    }
    if (_user_grade >= 0) {
        rsp.mutable_scoring_to_ors_msg()->set_user_grade(_user_grade);
        util::KeyValue* user_grade_kv = rsp.add_scoring_pvlogs();
        user_grade_kv->set_key("user_grade");
        user_grade_kv->set_value(util::Func::to_str(_user_grade));
    }
    if (_context_quality >= 0) {
        rsp.mutable_scoring_to_ors_msg()->set_context_quality(_context_quality);
        util::KeyValue* context_grade_kv = rsp.add_scoring_pvlogs();
        context_grade_kv->set_key("context_quality");
        std::ostringstream ss;
        ss << (int) (_context_quality < 0 ? -1 : _context_quality * 100);
        context_grade_kv->set_value(ss.str());
    }

    return 0;
}

}
}
