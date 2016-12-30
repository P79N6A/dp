#include "stat_rate_model.h"
#include "src/model_updater/api/structs.h"
#include "src/model_updater/api/algo_model_data_api.h"

namespace poseidon {
namespace scoring {

StatRateModel::StatRateModel() {
    _source = -1;
    _pid = -1;
    _os_type = -1;
    _advertiser_id = -1;
    _context_ctr = -1;
    _context_cvr = -1;
}

StatRateModel::~StatRateModel() {
}

int StatRateModel::Init() {
    return 0;
}

int StatRateModel::Fini() {
    return 0;
}

int StatRateModel::Prepare(QueryContext* qcontext) {
    using namespace model_updater;

    _context_ctr = 0.01;
    _context_cvr = 0.01;

    _source = qcontext->Source();
    _pid = qcontext->PrimaryPid();
    _os_type = qcontext->OsType();

    StatRateKey key;
    StatRateValue* value = NULL;

    key.source = _source;
    if (AlgoModelDataApi::get_mutable_instance().GetStatRateValue(key,
            &value)) {
        _context_ctr = value->ctr;
        _context_cvr = value->cvr;

        key.pid = _pid;
        if (AlgoModelDataApi::get_mutable_instance().GetStatRateValue(key,
                &value)) {
            _context_ctr = value->ctr;
            _context_cvr = value->cvr;

            key.os_type = _os_type;
            if (AlgoModelDataApi::get_mutable_instance().GetStatRateValue(key,
                    &value)) {
                _context_ctr = value->ctr;
                _context_cvr = value->cvr;
            }
        }
    }
    return 0;
}

float StatRateModel::Ctr(AdInfo* ad_info) {
    using namespace model_updater;

    float ctr = _context_ctr;
    StatRateKey key;
    StatRateValue* value = NULL;

    key.source = _source;
    key.os_type = _os_type;
    key.pid = _pid;
    key.view_type = ad_info->ViewType();
    while (1) {
        if (!AlgoModelDataApi::get_mutable_instance().GetStatRateValue(key,
                &value)) {
            break;
        } else {
            ctr = value->ctr;
        }
        key.campaign_id = ad_info->CampaignId();
        if (!AlgoModelDataApi::get_mutable_instance().GetStatRateValue(key,
                &value)) {
            break;
        } else {
            ctr = value->ctr;
        }
        key.ad_id = ad_info->AdId();
        if (!AlgoModelDataApi::get_mutable_instance().GetStatRateValue(key,
                &value)) {
            break;
        } else {
            ctr = value->ctr;
        }
        key.creative_id = ad_info->CreativeId();
        if (!AlgoModelDataApi::get_mutable_instance().GetStatRateValue(key,
                &value)) {
            break;
        } else {
            ctr = value->ctr;
            break;
        }
    }
    LOG_DEBUG("Scoring stat_rate key: %s", key.to_string());
    return ctr;
}

float StatRateModel::Cvr(AdInfo* ad_info) {
    using namespace model_updater;

    float cvr = _context_cvr;
    StatRateKey key;
    StatRateValue* value = NULL;

    key.source = _source;
    key.os_type = _os_type;
    key.pid = _pid;
    key.view_type = ad_info->ViewType();
    if (!AlgoModelDataApi::get_mutable_instance().GetStatRateValue(key,
            &value)) {
        return cvr;
    } else {
        cvr = value->cvr;
    }
    key.campaign_id = ad_info->CampaignId();
    if (!AlgoModelDataApi::get_mutable_instance().GetStatRateValue(key,
            &value)) {
        return cvr;
    } else {
        cvr = value->cvr;
    }
    key.ad_id = ad_info->AdId();
    if (!AlgoModelDataApi::get_mutable_instance().GetStatRateValue(key,
            &value)) {
        return cvr;
    } else {
        cvr = value->cvr;
    }
    key.creative_id = ad_info->CreativeId();
    if (!AlgoModelDataApi::get_mutable_instance().GetStatRateValue(key,
            &value)) {
        return cvr;
    } else {
        cvr = value->cvr;
        return cvr;
    }
}

}
}
