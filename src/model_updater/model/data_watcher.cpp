/**
 **/

//include STD C/C++ head files

//include third_party_lib head files
#include "src/model_updater/model/data_watcher.h"
#include "util/log.h"
#include "util/proto_helper.h"
#include "src/model_updater/model/base_param_handler.h"
#include "src/model_updater/model/budget_pacing_handler.h"
#include "src/model_updater/model/stat_rate_handler.h"
#include "src/model_updater/model/spot_grade_handler.h"
#include "src/model_updater/model/video_context_grade_handler.h"
#include "src/model_updater/model/lr_handler.h"
#include "src/model_updater/model/scoring_param_handler.h"
#include "src/model_updater/model/bidding_proposal_handler.h"
#include "src/model_updater/model/pay_factor_handler.h"

namespace poseidon {
namespace model_updater {

DataWatcher::DataWatcher() {
    m_loop = 0;
}

DataWatcher::~DataWatcher() {
    Fini();
}

bool DataWatcher::Init() {
    ModelUpdaterConfig config;
    std::string conf_file = "../etc/algo_model.conf";
    if (!util::ParseProtoFromTextFormatFile(conf_file.c_str(), &config))
    {
        LOG_ERROR("ParseProtoFromTextFormatFile Failed!conf_file=%s", conf_file.c_str());
        return false;
    }

    if (config.has_base_param_config()) {
        BaseParamHandler* base_param_handler = new BaseParamHandler();
        if (!base_param_handler->Init(config.base_param_config())) {
            LOG_ERROR("BaseParamHandler Init config Failed!");
            return false;
        }
        m_data_handlers.push_back(base_param_handler);
    }

    if (config.has_budget_pacing_config()) {
        BudgetPacingHandler* budget_pacing_handler = new BudgetPacingHandler();
        if (!budget_pacing_handler->Init(config.budget_pacing_config())) {
            LOG_ERROR("BudgetPacingHandler Init config Failed!");
            return false;
        }
        m_data_handlers.push_back(budget_pacing_handler);
    }

    if (config.has_stat_rate_config()) {
        StatRateHandler* stat_rate_handler = new StatRateHandler();
        if (!stat_rate_handler->Init(config.stat_rate_config())) {
            LOG_ERROR("StatRateHandler Init config Failed!");
            return false;
        }
        m_data_handlers.push_back(stat_rate_handler);
    }

    if (config.has_spot_grade_config()) {
        SpotGradeHandler* spot_grade_handler = new SpotGradeHandler();
        if (!spot_grade_handler->Init(config.spot_grade_config())) {
            LOG_ERROR("SpotGradeHandler Init config Failed!");
            return false;
        }
        m_data_handlers.push_back(spot_grade_handler);
    }

    if (config.video_context_grade_configs_size() > 0) {
        for (int i = 0; i < config.video_context_grade_configs_size(); i++) {
            VideoContextGradeHandler* video_context_grade_handler =
                new VideoContextGradeHandler();
            if (!video_context_grade_handler->Init(
                config.video_context_grade_configs(i))) {
                LOG_ERROR("VideoContextGradeHandler Init config Failed!");
                return false;
            }
            m_data_handlers.push_back(video_context_grade_handler);
        }
    }

    if (config.lr_configs_size() > 0) {
        for (int i = 0; i < config.lr_configs_size(); i++) {
            LRHandler* lr_handler = new LRHandler();
            if (!lr_handler->Init(config.lr_configs(i))) {
                LOG_ERROR("LRHandler Init config %d Failed!", i);
                return false;
            }
            m_data_handlers.push_back(lr_handler);
        }
    }

    if (config.has_scoring_param_config()) {
        ScoringParamHandler* scoring_param_handler = new ScoringParamHandler();
        if (!scoring_param_handler->Init(config.scoring_param_config())) {
            LOG_ERROR("ScoringParamHandler Init config Failed!");
            return false;
        }
        m_data_handlers.push_back(scoring_param_handler);
    }

    if (config.has_bidding_proposal_config()) {
        BiddingProposalHandler* bidding_proposal_handler = new BiddingProposalHandler();
        if (!bidding_proposal_handler->Init(config.bidding_proposal_config())) {
            LOG_ERROR("BiddingProposalHandler Init config Failed!");
            return false;
        }
        m_data_handlers.push_back(bidding_proposal_handler);
    }

    if (config.has_pay_factor_config()) {
        PayFactorHandler* pay_factor_handler = new PayFactorHandler();
        if (!pay_factor_handler->Init(config.pay_factor_config())) {
            LOG_ERROR("PayFactorHandler Init config Failed!");
            return false;
        }
        m_data_handlers.push_back(pay_factor_handler);
    }


    return true;
}

void DataWatcher::Fini() {
    std::vector<DataHandler*>::iterator iter = m_data_handlers.begin();
    std::vector<DataHandler*>::iterator end = m_data_handlers.end();
    for (; iter != end; iter++) {
        (*iter)->Fini();
        delete (*iter);
    }
    m_data_handlers.clear();
}

int DataWatcher::Run() {
    m_loop = 1;
    size_t handlers_size = m_data_handlers.size();

    if (handlers_size == 0) {
        LOG_ERROR("no handlers");
        return -1;
    }

    while (m_loop) {
        for (size_t i = 0; i < handlers_size; i++) {
            int ret = m_data_handlers[i]->Run();
            if (ret != 0) {
                LOG_ERROR("handle[%d] Run Failed!ret=%d", i, ret);
                continue;
            }
        }
        usleep(500 * 1000);
    }

    return 0;
}

void DataWatcher::Terminate() {
    for (size_t i = 0; i < m_data_handlers.size(); i++) {
        m_data_handlers[i]->Terminate();
    }
}

} // namespace model_updater
} // namespace poseidon

