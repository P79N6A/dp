#ifndef _ORS_OFFLINE_CXR_STATS_STAT_INFO_
#define _ORS_OFFLINE_CXR_STATS_STAT_INFO_

#include <cstddef>
#include <string>

namespace poseidon {
namespace ors_offline {

const std::string stat_delimiter = "`";

struct StatInfo {
public:
    StatInfo() {
        imprs = 0;
        costs = 0;
        costs_cvr = 0;
        clicks = 0;
        clicks_cvr = 0;
        binds = 0;

        ctr = -1;
        cvr = -1;

        cpm = -1;
        cpc = -1;
        cpa = -1;
        ecpa = -1;

        update_time = -1;
    }

    float imprs; //曝光
    float costs; //扣费，单位为元
    float costs_cvr; //扣费，单位为元，专为cpa使用
    float clicks; //点击
    float clicks_cvr; //点击，专为cvr使用
    float binds; //新增

    float ctr;
    float cvr;

    float cpm;
    float cpc;
    float cpa; //直接相除的cpa
    float ecpa; //层级加权的cpa

    int update_time; //更新时间：yyyymmdd

    //当统计不足时，不使用加权，因为ctr和cvr的统计情况不同，因此cvr部分数据单独处理
    //直接相加
    void RAdd(StatInfo* other);
    void RAddCvr(StatInfo* other);

    //加权平均
    void EAdd(StatInfo* other, float factor);
    void EAddCvr(StatInfo* other, float factor);

    void CalRRate(float default_ctr, float default_cvr, float default_ecpa);
    void CalERate(StatInfo* upper, float ctr_bayes_factor, float cvr_bayes_factor, float cpa_bayes_factor);
    void CalCpx();

    std::string SerializeEStatAsString();
    void DeserializeEStatFromString(const char* str);

};

}
}

#endif
