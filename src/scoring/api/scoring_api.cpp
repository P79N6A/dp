#include "scoring_api.h"

#include <fstream>
#include <string>

#include "protocol/src/poseidon_proto.h"
#include "util/log.h"
#include "util/proto_helper.h"
#include "src/monitor/api/monitor_api.h"
#include "src/scoring/common/common.h"
#include "src/scoring/context/params.h"

namespace poseidon {
namespace scoring {

const int kMaxDumpReqConut = 10000;

void dumpReq(const ScoringRequest & req, const std::string& path) {
    static int dump_count = 0;
    using namespace std;
    if (dump_count > kMaxDumpReqConut) {
        return;
    }
    char filepath[50];
    snprintf(filepath, 50, "%s_%d", path.c_str(), dump_count++);
    ofstream fout(filepath);
    fout << req.DebugString() << endl;
    fout.close();
}

int ScoringApi::Init(const std::string & conf_file) {

    try {
        CheckError(
                !util::ParseProtoFromTextFormatFile(conf_file.c_str(),
                        &_config), "Fail to load scoring config");
        if (_config.debug_mode()) {
            LOG_INFO("Scoring is disabled");
            return 0;
        }

        CheckError(Params::get_mutable_instance().Init(),
                "Fail to init scoring params");
        CheckError(_ad_context.Init(), "Fail to init scoring AdList");
        CheckError(_base_query_context.Init(),
                "Fail to init scoring query context");
        CheckError(_base_scorer.Init(), "Fail to init Scorer");
    } catch (int e) {
        return e;
    }
    return 0;
}

int ScoringApi::Proc(const ScoringRequest & req, ScoringResponse & rsp) {

    //单纯将请求dump下来
    if (_config.debug_mode()) {
        if (_config.has_dump_file()) {
            dumpReq(req, _config.dump_file());
            LOG_DEBUG("dump req to %s", _config.dump_file().c_str());
        }
        return 0;
    }

    //简单返回100个以下广告，保持链路运行
    if (_config.disable_mode()) {
        int rsp_ad_num = req.ad_size();
        rsp_ad_num = rsp_ad_num > 100 ? 100 : rsp_ad_num;
        for (int i = 0; i < rsp_ad_num; ++i) {
            rsp.add_ad()->CopyFrom(req.ad(i));
        }
        return 0;
    }

    //统计流量指标
    MON_ADD(ATTR_SVR_SCORING_REQ_COUNT, 1);
    MON_ADD(ATTR_SVR_SCORING_REQ_AD_NUM, req.ad_size());
    switch (req.traffic_source()) {
    case ADX_ID_YOUKU:
        MON_ADD(ATTR_SVR_SCORING_YOUKU_REQ_COUNT, 1);
        break;
    case ADX_ID_YUNOS:
        MON_ADD(ATTR_SVR_SCORING_YUNOS_REQ_COUNT, 1);
        break;
    case ADX_ID_IQIYI:
        MON_ADD(ATTR_SVR_SCORING_IQIYI_REQ_COUNT, 1);
        break;
    case ADX_ID_TOUTIAO:
        MON_ADD(ATTR_SVR_SCORING_TOUTIAO_REQ_COUNT, 1);
        break;
    }

    try {
        CheckError(prepare(req, rsp), "Fail to prepare scoring");

        CheckError(_scorer->AnalyzeContext(_qcontext),
                "Fail to analyze scoring context");

        CheckError(_scorer->FillOrsMsgResponse(rsp),
                "Fail to fill scoring response");

        if (!_scorer->FilterRequest(_qcontext, &_ad_context)) {
            LOG_DEBUG("Filter out scoring request");
            return 0;
        }
        MON_ADD(ATTR_SVR_SCORING_FILTER_REQ_COUNT, 1);
        LOG_DEBUG("Filter scoring request");

        CheckError(_scorer->FilterAdByParam(&_ad_context),
                "Fail to filter ad by scoring param");

        CheckError(_scorer->IncludeAdByParam(&_ad_context),
                "Fail to include ad by scoring param");

        CheckError(_scorer->ScoringAds(_qcontext, &_ad_context),
                "Fail to scoring ads");

        CheckError(_scorer->ChooseFinalAds(&_ad_context, rsp),
                "Fail to choose ads to fill scoring response");
        MON_ADD(ATTR_SVR_SCORING_RSP_AD_NUM, rsp.ad_size());

    } catch (int e) {
        MON_ADD(ATTR_SVR_SCORING_ERROR_COUNT, 1);
        return e;
    }

    return 0;

}

int ScoringApi::Fini() {
    _ad_context.Fini();
    _base_query_context.Fini();
    _base_scorer.Fini();
    return 0;
}

int ScoringApi::prepare(const scoring::ScoringRequest & req,
        ScoringResponse& rsp) {
    _scorer = &_base_scorer;
    _qcontext = &_base_query_context;
    Params::get_mutable_instance().UpdateParams();

    try {
        CheckError(_ad_context.ResetContext(req), "Fail to reset ad context");
        CheckError(_qcontext->ResetContext(req), "Fail to reset query context");
        CheckError(_scorer->Prepare(_qcontext), "Fail to prepare scorer");

    } catch (int e) {
        return e;
    }

    return 0;
}

}
}
