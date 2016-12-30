#include "stat_info.h"
#include <cstdio>
#include <sstream>
#include <iomanip>
#include "src/ors_offline/common/utility.h"

namespace poseidon {
namespace ors_offline {

//平滑函数
float CalEffectRate(float u, float d, float refer_rate, float factor) {
    if (u > d) {
        u = 1;
        d = 1;
    }
    if (refer_rate < 0 || refer_rate > 1) {
        return -1;
    }
    return (u + factor) / (d + factor / refer_rate);
}

void StatInfo::RAdd(StatInfo* other) {
    if (NULL == other) {
        return;
    }
    imprs += other->imprs;
    costs += other->costs;
    clicks += other->clicks;
    update_time =
            (update_time > other->update_time) ? update_time :
                                                 other->update_time;
}

void StatInfo::RAddCvr(StatInfo* other) {
    if (NULL == other) {
        return;
    }
    costs_cvr += other->costs_cvr;
    clicks_cvr += other->clicks_cvr;
    binds += other->binds;
    update_time =
            (update_time > other->update_time) ? update_time :
                                                 other->update_time;
}

void StatInfo::EAdd(StatInfo* other, float factor) {
    if (NULL == other) {
        return;
    }
    imprs = imprs * (1 - factor) + other->imprs * factor;
    costs = costs * (1 - factor) + other->costs * factor;
    clicks = clicks * (1 - factor) + other->clicks * factor;
    update_time =
            (update_time > other->update_time) ? update_time :
                                                 other->update_time;
}

void StatInfo::EAddCvr(StatInfo* other, float factor) {
    if (NULL == other) {
        return;
    }
    costs_cvr = costs_cvr * (1 - factor) + other->costs_cvr * factor;
    clicks_cvr = clicks_cvr * (1 - factor) + other->clicks_cvr * factor;
    binds = binds * (1 - factor) + other->binds * factor;
    update_time =
            (update_time > other->update_time) ? update_time :
                                                 other->update_time;
}

void StatInfo::CalRRate(float default_ctr, float default_cvr, float default_ecpa) {
    ctr = (0 == imprs || clicks >= imprs * 0.3) ? default_ctr : clicks / imprs;
    cvr = (0 == clicks_cvr || binds >= clicks_cvr * 0.3) ? default_cvr : binds / clicks_cvr;
    ecpa = (0 == binds) ? default_ecpa : costs_cvr / binds;
}

void StatInfo::CalERate(StatInfo* upper, float ctr_bayes_factor,
        float cvr_bayes_factor, float cpa_bayes_factor) {
    if (NULL == upper) {
        ctr = -1;
        cvr = -1;
        ecpa = -1;
    } else {
        ctr = CalEffectRate(clicks, imprs, upper->ctr, ctr_bayes_factor);
        cvr = CalEffectRate(binds, clicks_cvr, upper->cvr, cvr_bayes_factor);
        ecpa = 1
                / CalEffectRate(binds, costs_cvr, 1 / upper->ecpa,
                        cpa_bayes_factor);
    }
}

void StatInfo::CalCpx() {
    cpm = (0 == imprs) ? 0 : costs / imprs * 1000;
    cpc = (0 == clicks) ? 0 : costs / clicks;
    cpa = (0 == binds) ? 0 : costs_cvr / cpa;
}

std::string StatInfo::SerializeEStatAsString() {
    using namespace std;
    stringstream ss;
    //统计计数值
    ss << fixed << setprecision(2) << imprs;
    ss << stat_delimiter << fixed << setprecision(2) << costs;
    ss << stat_delimiter << fixed << setprecision(2) << costs_cvr;
    ss << stat_delimiter << fixed << setprecision(2) << clicks;
    ss << stat_delimiter << fixed << setprecision(2) << clicks_cvr;
    ss << stat_delimiter << fixed << setprecision(2) << binds;
    ss << stat_delimiter << update_time;
    //计算指标，不反序列化
    ss << "\t" << fixed << setprecision(4) << ctr;
    ss << stat_delimiter << fixed << setprecision(4) << cvr;
    ss << stat_delimiter << fixed << setprecision(2) << ecpa;
    return ss.str();
}

void StatInfo::DeserializeEStatFromString(const char* str) {
    sscanf(str, "`%f`%f`%f`%f`%f`%f`%d", &imprs, &costs, &costs_cvr,
            &clicks, &clicks_cvr, &binds, &update_time);
}

}
}
