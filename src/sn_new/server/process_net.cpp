#include "process_net.h"
#include <boost/algorithm/string.hpp>
#include "util/log.h"
#include "util/func.h"
#include "util/monitor.h"
#include "monitor_api.h"
#include "sn_attr.h"
#include "protocol/src/poseidon_proto.h"
#include "api/exp_api.h"
#include "config.h"
#include <unistd.h>

#define EXP_MODULE_ID 10002
#define MAX_RETURN_SIZE 64
#define TV_SUB_US(_tse_, _tsb_) \
    ((_tse_.tv_sec * 1e6 + _tse_.tv_usec) - (_tsb_.tv_sec * 1e6 + _tsb_.tv_usec))

namespace poseidon {
namespace sn {

enum rc_e {
    RC_OK = 0,
    RC_CALL_EXPAPI_ERR,
    RC_INIT_EXPAPI_ERR,
    RC_INIT_INVERTED_INDEX_ERR,
    RC_INIT_SCORING_API_ERR,
    RC_NO_ADZ_INFO,
    RC_OOM_ERR,
    RC_QUERY_INDEX_ERR,
    RC_SCORING_API_ERR,
    RC_SEND_PKG_ERR,
    RC_SERIALIZE_SN_RSP_ERR,
    RC_SN_REQ_PARSE_ERR,
    RC_MAX
};

static const char *rc_msg[RC_MAX] = {
    "RC_OK",
    "RC_CALL_EXPAPI_ERR",
    "RC_INIT_EXPAPI_ERR",
    "RC_INIT_INVERTED_INDEX_ERR",
    "RC_INIT_SCORING_API_ERR",
    "RC_NO_ADZ_INFO",
    "RC_OOM_ERR",
    "RC_QUERY_INDEX_ERR",
    "RC_SCORING_API_ERR",
    "RC_SEND_PKG_ERR",
    "RC_SERIALIZE_SN_RSP_ERR",
    "RC_SN_REQ_PARSE_ERR"
};

enum
{
    CITY_TARG_ID=1007,
    VIEW_TYPE_TARG_ID=19001,
    SOURCE_TARG_ID=19002,
    DEAL_ID_TARG_ID=19003,
};

static inline const char *strrc(int rc)
{
    rc = rc < 0 ? -rc : rc;
    return rc_msg[rc];
}

struct SessData
{
    int adcnt_inv_index;
    int adcnt_filter_ad;
    int adcnt_scor;
    uint32_t user_city_id;
    uint64_t time_build_query;
    uint64_t time_exp_api;
    uint64_t time_query_inv_index;
    uint64_t time_filter_ad;
    uint64_t time_scor_api;
    uint64_t time_package_rsp;
    uint64_t time_seri_rsp;
    uint64_t time_send_pkg;
    uint64_t time_req2rsp;
    SNRequest sn_req;
    SNResponse sn_rsp;
    scoring::ScoringRequest scor_req;
    scoring::ScoringResponse scor_rsp;
};

static int InitInvTargets(const char *str, std::set<int> &targets)
{
    using namespace std;

    string tgs(str);
    set<string> vrstr;
    boost::split(vrstr, tgs, boost::is_any_of(","));

    set<string>::iterator sit;
    for (sit = vrstr.begin(); sit != vrstr.end(); ++sit) {
        targets.insert(atoi(sit->c_str()));
    }

    int rc = 0;
    do {
        if (targets.size() == 0) {
            rc = -1;
            break;
        }

        if (targets.size() > 1) break;

        set<int>::iterator it = targets.begin();
        if (0 == *it) {
            rc = -1;
            break;
        }
    } while(0);

    if (rc < 0) {
        LOG_ERROR("no targets for inverted index");
    }

    return rc;
}

int ProcessNet::init() 
{
    using std::string;

    int rt = 0, rc = RC_OK;
    if (init_) return 0;

    do {
        rt = InitInvTargets(Config::get_mutable_instance().inv_targets(), inv_targets_); 
        if (rt != 0) {
            MON_ADD(ATTR_INVERTED_INDEX_ERR, 1);
            rc = -RC_INIT_INVERTED_INDEX_ERR;
            break;
        }

        rt = inverted_index_.Init();
        if (rt != 0) {
            MON_ADD(ATTR_INVERTED_INDEX_ERR, 1);
            rc = -RC_INIT_INVERTED_INDEX_ERR;
            break;
        }

        const string &scor_conf = string(Config::get_mutable_instance().scoring_file());
        scoring_api_ = &scoring::ScoringApi::get_mutable_instance();
        rt = scoring_api_->Init(scor_conf);
        if (rt != 0) {
            MON_ADD(ATTR_SCORING_ERR, 1);
            rc = -RC_INIT_SCORING_API_ERR;
            break;
        }

        init_ = true;
    } while(0);

    if (rc < 0) {
        LOG_ERROR("%s, rc=%d", strrc(rc), rt);
    }

    return rc;
}

static int CallExpApi(SessData *sess)
{
    using namespace std;
    using namespace exp_sys;
    using namespace scoring;

    int rt = 0, rc = RC_OK;
    SNRequest &sn_req = sess->sn_req;
    SNResponse &sn_rsp = sess->sn_rsp;
    ScoringRequest &scor_req = sess->scor_req;
    struct timeval time_beg, time_end;
    gettimeofday(&time_beg, NULL);

    do {
        ExpApi &ex_api = ExpApi::get_mutable_instance();
        if (!ex_api.init()) { 
            rc = -RC_INIT_EXPAPI_ERR; 
            break;
        }

        vector<int> exp_ids;
        vector<common::ExpParam> exp_params;
        const AdzInfo &adz = sn_req.adz_info(0);

        int count = adz.view_types_size();
        if (count == 0 && adz.has_view_type()) count = 1;

        for (int i = 0; i < count; ++i) {
            vector<int> exp_id;
            vector<exp_sys::ExpParam> exp_param;
            int vt = adz.view_types_size() > 0 ? adz.view_types(i) : adz.view_type();

            rt = ex_api.get_exp_param(EXP_MODULE_ID, sn_req.traffic_source(), 
                                      vt, exp_id, exp_param);

            if (rt != 0) { 
                rc = -RC_CALL_EXPAPI_ERR; 
                break;
            }

            exp_ids.insert(exp_ids.begin(), exp_id.begin(), exp_id.end());

            for (int i = 0; i < (int)exp_param.size(); ++i) {
                const exp_sys::ExpParam &srcParam = exp_param.at(i);
                common::ExpParam dstParam;
                dstParam.set_param_id(srcParam.param_id);
                dstParam.set_view_type(vt);
                
                if (srcParam.param_type == PT_INT) {
                    dstParam.set_int_value(srcParam.param_vlaue.int_v);
                } else {
                    dstParam.set_float_value(srcParam.param_vlaue.float_v);
                }
               
                exp_params.push_back(dstParam);
            }
        }

        if (rc < 0) break;

        vector<common::ExpParam>::iterator exp_iter;
        for (exp_iter = exp_params.begin(); exp_iter != exp_params.end(); ++exp_iter) {
            const common::ExpParam &src_exp = *exp_iter;
            common::ExpParam *dest_exp = scor_req.add_exp_param();
            dest_exp->CopyFrom(src_exp);
        }

        vector<int>::iterator exid_iter;
        for (exid_iter = exp_ids.begin(); exid_iter != exp_ids.end(); ++exid_iter) {
            LOG_DEBUG("exp_id: %d", *exid_iter);
            sn_rsp.add_exp_id(*exid_iter);
        }

    } while(0);

    if (rc < 0) {
        MON_ADD(ATTR_EXP_ERR, 1);
        LOG_ERROR("%s, rc=%d", strrc(rc), rt);
    }

    gettimeofday(&time_end, NULL);
    sess->time_exp_api = TV_SUB_US(time_end, time_beg);

    return rc;
}

/**
 * @brief               过滤广告
 * @param sess          [IN], Session
 * @param adz           [IN],广告位信息
 * @param ad            [IN],待过滤的广告
 * @return              允许广告返回true, 否则返回false
 **/
static bool FilterAd(const SessData *sess, const sn::AdzInfo &adz, const common::Ad &ad)
{
    struct tm tminfo;
    util::Func::get_time_info(tminfo);

    /*第nseg个时段, 半个小时一个时段*/
    int nseg = tminfo.tm_hour * 2 + (tminfo.tm_min >= 30 ? 1 : 0);
    int64_t flag = ad.post_hours() & (((int64_t)1) << nseg);

    //是否是投放时段
    if (flag == 0) return false;

    //所在城市过滤, 广告没有城市定向，则放过
    if (ad.post_region_size() <= 0) return true;

    //用户没有城市定向数据，广告有，则过滤
    if (sess->user_city_id == 0) return false;

    // xx        xx    xx
    // |         |     |
    // province  city  area
    int user_prov = sess->user_city_id / 10000;
    int user_city = (sess->user_city_id - user_prov * 10000) / 100;
    int user_area = sess->user_city_id - user_prov * 10000 - user_city * 100;

    for (int i = 0; i < ad.post_region_size(); i++) {
        //province + city + area
        int ad_city = ad.post_region(i);

        //判断用户所在城市是否在广告的定向城市列表
        if (ad_city / 10000 != user_prov) continue;

        //after assignment, ad_city => city + area
        ad_city = ad_city % 10000;
        if (ad_city == 0) return true;

        if (ad_city / 100 != user_city) continue;

        //after assignment, ad_city => area
        ad_city = ad_city % 100;
        if (ad_city == 0 || ad_city == user_area) return true;
    }

    return false;
}

static void stat_view_type(size_t vtcnt)
{
    if (vtcnt <= 10) {
        MON_ADD(ATTR_VIEW_TYPE_CNT_0 + vtcnt, 1);
    } else {
        MON_ADD(ATTR_VIEW_TYPE_CNT_GT_10, 1);
    }
}

int ProcessNet::BuildQuery(SessData *sess, std::vector<Query> &queries) 
{
    int rt = 0;
    TagType tvalue;
    std::vector<int64_t> vtypes;
    SNRequest &sn_req = sess->sn_req;
    const AdzInfo &adz = sn_req.adz_info(0);
    int tar_size = adz.targetting_size();

    for (int i = 0; i < adz.view_types_size(); i++) {
        int32_t vt = adz.view_types(i);
        tvalue = (int64_t(VIEW_TYPE_TARG_ID) << 32) | vt;
        vtypes.push_back(tvalue);
    }

    // view_types first instead of single view_type
    if (vtypes.size() == 0 && adz.has_view_type()) {
        tvalue = (int64_t(VIEW_TYPE_TARG_ID) << 32) | adz.view_type();
        vtypes.push_back(tvalue);
    }

    int i = 0;
    do {
        Query query;

        sess->user_city_id = 0; //默认0，表示没有该用户城市信息
        for (int idx = 0; idx < tar_size; idx++) {
            const common::Targetting &tar = adz.targetting(idx);
            int type = tar.type();
            if (tar.value_size() == 0) { continue; }

            std::string value = tar.value(0);
            int intvalue = atoi(value.c_str());

            //用户所在地不作为检索条件
            if(type == CITY_TARG_ID) {
                sess->user_city_id = intvalue;
                continue;
            }

            //no need query ad for this target
            if (inv_targets_.find(type) == inv_targets_.end()) continue;

            tvalue = (int64_t(type) << 32) | intvalue;
            query.push_back(tvalue);
        }

        if (sn_req.has_traffic_source()) {
            tvalue = (int64_t(SOURCE_TARG_ID) << 32) | sn_req.traffic_source();
            query.push_back(tvalue);
        }

        if (adz.has_deal_id()) {
            tvalue = (int64_t(DEAL_ID_TARG_ID) << 32) | util::Func::to_int(adz.deal_id());
            query.push_back(tvalue);
        } else {
            tvalue = (int64_t(DEAL_ID_TARG_ID) << 32) | 0;
            query.push_back(tvalue);
        }

        if (i < (int)vtypes.size()) {
            query.push_back(vtypes.at(i));
        }

        queries.push_back(query);

    } while(++i < (int)vtypes.size());

    stat_view_type(vtypes.size());

    return rt;
}

int ProcessNet::QueryAndFilter(SessData *sess, std::vector<common::Ad> &target_ads)
{
    using namespace std;
    using namespace common;

    int rt = 0, rc = RC_OK;
    struct timeval time_beg, time_end;
    list<Ad> ads;
    vector<Query> queries;
    vector<Query>::iterator q_iter;

    SNRequest &sn_req = sess->sn_req;
    const AdzInfo &adz_info = sn_req.adz_info(0);

    LOG_DEBUG("build query");
    gettimeofday(&time_beg, NULL);
    BuildQuery(sess, queries);
    gettimeofday(&time_end, NULL);
    sess->time_build_query = TV_SUB_US(time_end, time_beg);

    LOG_DEBUG("query inverted index, query count=%zd", queries.size());
    for (q_iter = queries.begin(); q_iter != queries.end(); ++q_iter) {
        Query &q = *q_iter;
        list<Ad> sub_ads;

        rt = inverted_index_.QueryIndex(q, sub_ads);
        if (rt != 0) {
            MON_ADD(ATTR_INVERTED_INDEX_ERR, 1);
            rc = -RC_QUERY_INDEX_ERR;
            break;
        }

        ads.splice(ads.end(), sub_ads);
    }
    gettimeofday(&time_beg, NULL);
    sess->time_query_inv_index = TV_SUB_US(time_beg, time_end);
    sess->adcnt_inv_index = ads.size();

    while (rc == RC_OK) {
        list<Ad>::iterator ad_iter;
        for (ad_iter = ads.begin(); ad_iter != ads.end(); ++ad_iter) {
            const Ad &ad = *ad_iter;
            bool allow = FilterAd(sess, adz_info, ad);
            if (!allow) { continue; }

            target_ads.push_back(ad);
        }

        break;
    }

    if (rc < 0) {
        LOG_ERROR("%s, rc=%d", strrc(rc), rt);
    }

    gettimeofday(&time_end, NULL);
    sess->time_filter_ad = TV_SUB_US(time_end, time_beg);
    sess->adcnt_filter_ad = target_ads.size();

    return rc;
}

int ProcessNet::CallScoringApi(SessData *sess, std::vector<common::Ad> &target_ads)
{
    using namespace std;
    using namespace common;
    using namespace scoring;

    int rt, rc = RC_OK;

    SNRequest &sn_req = sess->sn_req;
    ScoringRequest &scor_req = sess->scor_req;
    ScoringResponse &scor_rsp = sess->scor_rsp;
    const AdzInfo &src_adz = sn_req.adz_info(0);

    scor_req.add_adz_info()->CopyFrom(src_adz);
    scor_req.set_traffic_source(sn_req.traffic_source());
    scor_req.set_allocated_time_info(sn_req.mutable_time_info());
    scor_req.set_allocated_geo(sn_req.mutable_geo());
    scor_req.set_allocated_session_id(sn_req.mutable_session_id());
    scor_req.set_allocated_trace_id(sn_req.mutable_trace_id());
    scor_req.set_allocated_app_info(sn_req.mutable_app_info());
    scor_req.set_allocated_device_info(sn_req.mutable_device_info());

    vector<Ad>::iterator ad_iter;
    for (ad_iter = target_ads.begin(); ad_iter != target_ads.end(); ++ad_iter) {
        const Ad &ad = *ad_iter;
        LOG_DEBUG("%s", ad.DebugString().c_str());
        scor_req.add_ad()->CopyFrom(ad);
    }

    struct timeval time_beg, time_end;
    gettimeofday(&time_beg, NULL);
    rt = scoring_api_->Proc(scor_req, scor_rsp);
    if(rt != 0) {
        MON_ADD(ATTR_SCORING_ERR, 1);
        rc = -RC_SCORING_API_ERR;
    }

    gettimeofday(&time_end, NULL);
    sess->time_scor_api = TV_SUB_US(time_end, time_beg);
    sess->adcnt_scor = scor_rsp.ad_size();

    scor_req.release_time_info();
    scor_req.release_geo();
    scor_req.release_session_id();
    scor_req.release_trace_id();
    scor_req.release_app_info();
    scor_req.release_device_info();

    if (rc < 0) {
        LOG_ERROR("%s, rt=%d", strrc(rc), rt);
    }

    return rc;
}

static int PackageSnRsp(SessData *sess)
{
    using namespace std;
    using namespace common;
    using namespace scoring;

    int rc = RC_OK;
    struct timeval time_beg, time_end;

    SNRequest &sn_req = sess->sn_req;
    SNResponse &sn_rsp = sess->sn_rsp;
    ScoringResponse &scor_rsp = sess->scor_rsp;
    gettimeofday(&time_beg, NULL);

    sn_rsp.set_err_code(ERROR_NONE);
    sn_rsp.set_session_id(sn_req.session_id());
    sn_rsp.set_trace_id(sn_req.trace_id());

    if (scor_rsp.has_scoring_to_ors_msg()) {
        sn_rsp.mutable_scoring_to_ors_msg()->CopyFrom(scor_rsp.scoring_to_ors_msg());
    }

    if (scor_rsp.ad_size() == 0) {   
        MON_ADD(ATTR_SN_RETURN_ZERO, 1); 
    } else {   
        MON_ADD(ATTR_SN_RETURN_NOT_ZERO, 1); 
    }   

    if (scor_rsp.ad_size() >= MAX_RETURN_SIZE) {   
        MON_ADD(ATTR_SN_RETURN_MAX, 1); 
    }   

    Ads *pAds = sn_rsp.add_ads();
    pAds->set_err_code(ERROR_NONE);
    for (int i = 0; i < MAX_RETURN_SIZE && i < scor_rsp.ad_size(); ++i) {
        Ad *ad = pAds->add_ad();
        ad->CopyFrom(scor_rsp.ad(i));
    }

    for (int i = 0; i < scor_rsp.scoring_pvlogs_size(); ++i) {
        sn_rsp.add_scoring_pvlogs()->CopyFrom(scor_rsp.scoring_pvlogs(i));
    }

    char hostname[256], fmtstr[256];
    memset(hostname, 0x00, 256); 
    gethostname(hostname, 256);
    snprintf(fmtstr, sizeof(fmtstr), "%s_%d", hostname, getpid());
    sn_rsp.set_hostname(fmtstr);

    gettimeofday(&time_end, NULL);
    sess->time_package_rsp = TV_SUB_US(time_end, time_beg);

    return rc;
}

int ProcessNet::Reply(SessData *sess, struct sockaddr_in &client_addr)
{
    using namespace std;

    int rt = 0, rc = RC_OK;
    struct timeval time_beg, time_end;
    SNResponse &sn_rsp = sess->sn_rsp;

    do {
        gettimeofday(&time_beg, NULL);
        string rspstr;
        if (!sn_rsp.SerializeToString(&rspstr)) {   
            rc = -RC_SERIALIZE_SN_RSP_ERR;
            break;
        }   
        gettimeofday(&time_end, NULL);
        sess->time_seri_rsp = TV_SUB_US(time_end, time_beg);

        LOG_DEBUG("snrsp[%s], rsp.ByteSize[%u], rspstr.length[%u]", 
                sn_rsp.DebugString().c_str(), sn_rsp.ByteSize(), rspstr.length());

        gettimeofday(&time_beg, NULL);
        rt = send_pkg(rspstr.c_str(), rspstr.length(), client_addr);
        if (rt != 0) {
            rc = -RC_SEND_PKG_ERR;
            break;
        }
        sess->time_send_pkg = TV_SUB_US(time_beg, time_end);

    } while(0);

    if (rc < 0) {
        MON_ADD(ATTR_RSP_ERR, 1);
        LOG_ERROR("%s, rc=%d", strrc(rc), rt);
    } 

    return rc;
}
 
static void stat_latency(SessData *sess) {
    uint64_t latency = sess->time_req2rsp / 1000;
    if(latency < 5) {
        MON_ADD(ATTR_SN_LATENCY_LESS_5, 1);
    } else if(latency < 10) {
        MON_ADD(ATTR_SN_LATENCY_LESS_10, 1);
    } else if(latency < 20) {
        MON_ADD(ATTR_SN_LATENCY_LESS_20, 1);
    } else {
        MON_ADD(ATTR_SN_LATENCY_GT_20, 1);
    }

    latency = sess->time_query_inv_index / 1000;
    if(latency < 5) {
        MON_ADD(ATTR_INVIDX_LATENCY_LESS_5, 1);
    } else if(latency < 10) {
        MON_ADD(ATTR_INVIDX_LATENCY_LESS_10, 1);
    } else if(latency < 20) {
        MON_ADD(ATTR_INVIDX_LATENCY_LESS_20, 1);
    } else {
        MON_ADD(ATTR_INVIDX_LATENCY_GT_20, 1);
    }

    latency = sess->time_scor_api / 1000;
    if(latency < 5) {
        MON_ADD(ATTR_SCOR_LATENCY_LESS_5, 1);
    } else if(latency < 10) {
        MON_ADD(ATTR_SCOR_LATENCY_LESS_10, 1);
    } else if(latency < 20) {
        MON_ADD(ATTR_SCOR_LATENCY_LESS_20, 1);
    } else {
        MON_ADD(ATTR_SCOR_LATENCY_GT_20, 1);
    }
}

int ProcessNet::DoRequest(SessData *sess)
{
    using namespace std;
    using namespace common;

    int rc = RC_OK;
    SNRequest &sn_req = sess->sn_req;

    int adz_size = sn_req.adz_info_size();
    if (adz_size <= 0) {
        MON_ADD(ATTR_REQ_PARSE_ERR, 1);
        return -RC_NO_ADZ_INFO;
    } else if (adz_size > 1) {
        MON_ADD(ATTR_REQ_MULTI_ADZ, 1);
    }

    vector<Ad> target_ads;

    /* step1: 获取实验参数 */
    LOG_DEBUG("call exp_api");
    rc = CallExpApi(sess);
    if (rc != 0) return rc;

    /*step 2:查询倒排索引并过滤*/
    LOG_DEBUG("query and filter ads");
    rc = QueryAndFilter(sess, target_ads);
    if (rc != 0) return rc;

    /*step 3: 调用scoring*/
    LOG_DEBUG("call scoring");
    rc = CallScoringApi(sess, target_ads);
    if (rc != 0) return rc;

    return rc;
}

int ProcessNet::handle_read(const char * buf, const int len, 
        struct sockaddr_in & client_addr) {

    using namespace std;
    using namespace scoring;
    using namespace common;

    int rc = RC_OK;
    struct timeval time_beg, time_end;
    SessData *sess = NULL;
    gettimeofday(&time_beg, NULL);

    do {
        MON_ADD(ATTR_SN_REQ, 1);
        sess = new(std::nothrow) SessData();
        if (sess == NULL) { 
            MON_ADD(ATTR_INTERNAL_ERR, 1);
            rc = -RC_OOM_ERR;
            break; 
        }

        SNRequest &sn_req = sess->sn_req;

        if (!sn_req.ParseFromArray(buf, len)) {
            MON_ADD(ATTR_REQ_PARSE_ERR, 1);
            rc = -RC_SN_REQ_PARSE_ERR;
            break;
        }

        LOG_DEBUG("recv sn req[%s]", sn_req.DebugString().c_str());

        rc = DoRequest(sess);
        if (rc != 0) { break; }

        LOG_DEBUG("package sn rsp");
        rc = PackageSnRsp(sess);
        if (rc != 0) { break; }

        LOG_DEBUG("do reply");
        rc = Reply(sess, client_addr);
        if (rc != 0) { break; }

        gettimeofday(&time_end, NULL);
        sess->time_req2rsp = TV_SUB_US(time_end, time_beg);

        MON_ADD(ATTR_SN_RSP, 1); 
        stat_latency(sess);

    } while(0);

    MON_ADD(ATTR_INVERTED_ADS_CNT, sess->adcnt_inv_index);
    MON_ADD(ATTR_FILTER_ADS_CNT, sess->adcnt_filter_ad);
    MON_ADD(ATTR_SCORING_ADS_CNT, sess->adcnt_scor);

    LOG_DEBUG("\n====elapse time[us]====\n"
              "time_req2rsp: %llu,\n"
              "time_build_query: %llu,\n"
              "time_exp_api: %llu,\n"
              "time_query_inv_index: %llu,\n"
              "time_filter_ad: %llu,\n"
              "time_scor_api: %llu,\n"
              "time_package_rsp: %llu,\n"
              "time_seri_rsp: %llu,\n"
              "time_send_pkg: %llu,\n"
              "\n====AD count after call====\n"
              "adcnt_inv_index: %d,\n" 
              "adcnt_filter_ad: %d,\n" 
              "adcnt_scor: %d,\n", 
              sess->time_req2rsp,
              sess->time_build_query,
              sess->time_exp_api,
              sess->time_query_inv_index,
              sess->time_filter_ad,
              sess->time_scor_api,
              sess->time_package_rsp,
              sess->time_seri_rsp,
              sess->time_send_pkg,
              sess->adcnt_inv_index,
              sess->adcnt_filter_ad,
              sess->adcnt_scor
            );

    if (rc < 0) {
        LOG_ERROR("%s\nreq[%s]", strrc(rc), 
                sess == NULL ? "" : sess->sn_req.DebugString().c_str());
    } 

    if (sess != NULL) { delete sess; }

    return rc;
}

#ifdef FOR_UNIT_TEST
void ProcessNet::TestRequest(SessData *sess)
{
    DoRequest(sess);
}
#endif

}
}

