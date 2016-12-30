#ifndef SRC_SCORING_COMMON_COMMON_H_
#define SRC_SCORING_COMMON_COMMON_H_

#include <stdarg.h>
#include <vector>
#include <boost/locale/encoding_utf.hpp>
#include "util/log.h"
#include "util/func.h"

namespace poseidon {

namespace scoring {

enum AdxID {
    ADX_ID_TANX = 1,
    ADX_ID_YOUKU = 2,
    ADX_ID_ALIGAME = 3,
    ADX_ID_CHANCE = 4,
    ADX_ID_YUNOS = 5,
    ADX_ID_IQIYI = 6,
    ADX_ID_TOUTIAO = 7,
    ADX_ID_MAX_NUM = 8
};

enum BillingType {
    CPT = 1, CPA = 2, CPC = 3, CPM = 4, CPD = 5
};

enum VideoSiteId {
    VIDEO_SITE_YOUKU = 1, VIDEO_SITE_TUDOU = 2, VIDEO_SITE_IQIYI = 3
};

enum UserTagType {
    USER_TAG_TYPE_USER_GRADE = 5001, USER_TAG_TYPE_SEED_USER = 5002
};

//monitor
#define ATTR_SVR_SCORING_REQ_COUNT  2254
#define ATTR_SVR_SCORING_FILTER_REQ_COUNT   2255
#define ATTR_SVR_SCORING_REQ_AD_NUM 2256
#define ATTR_SVR_SCORING_RSP_AD_NUM 2257
#define ATTR_SVR_SCORING_ERROR_COUNT    2258
#define ATTR_SVR_SCORING_NO_VIDEO_GRADE_COUNT   2259
#define ATTR_SVR_SCORING_NO_CONTEXT_GRADE_COUNT 2260
#define ATTR_SVR_SCORING_NO_USER_GRADE_COUNT    2261
#define ATTR_SVR_SCORING_HAS_SEED_USER_GRADE_COUNT  2262
#define ATTR_SVR_SCORING_REQ_NO_AD_COUNT 2402
#define ATTR_SVR_SCORING_FILTER_BY_USER_SEED 2403
#define ATTR_SVR_SCORING_FILTER_BY_CONTEXT  2404
#define ATTR_SVR_SCORING_FILTER_BY_USER_GRADE 2405
#define ATTR_SVR_SCORING_FILTER_BY_PDB 2406

#define ATTR_SVR_SCORING_YOUKU_USE_VID  2407
#define ATTR_SVR_SCORING_YOUKU_USE_SHOWID  2408
#define ATTR_SVR_SCORING_YOUKU_USE_TITLE    2409
#define ATTR_SVR_SCORING_YOUKU_USE_KEYWORD  2410

#define ATTR_SVR_SCORING_IQIYI_USE_VID  2411
#define ATTR_SVR_SCORING_IQIYI_USE_SHOWID  2412
#define ATTR_SVR_SCORING_IQIYI_USE_TITLE    2413
#define ATTR_SVR_SCORING_IQIYI_USE_KEYWORD  2414

#define ATTR_SVR_SCORING_YOUKU_REQ_COUNT    2418
#define ATTR_SVR_SCORING_IQIYI_REQ_COUNT    2419
#define ATTR_SVR_SCORING_TOUTIAO_REQ_COUNT    2420
#define ATTR_SVR_SCORING_YUNOS_REQ_COUNT    2421

//exp
#define EXP_PARAM_SCORING_USER_GRADE_PLAN_ID 8

#define USER_GRADE_LOW_LIMIT 109
#define USER_GRADE_HIGH_LIMIT 101

//seed user的字典函数
inline int SeedUserCode(int seed_cate) {
    switch (seed_cate) {
    case 1001:
        return 10; //这个值被用于判断，需要跟对应代码一同改动
    case 1101:
        return 9;
    case 1102:
        return 8;
    case 1103:
        return 7;
    }
    return -1;
}

inline void CheckError(int result, const char* fmt, ...) {
    if (0 == result) {
        return;
    }
    if (NULL != fmt) {
        char str_info[1000] = { '\0' };
        va_list arg_ptr;
        va_start(arg_ptr, fmt);
        vsnprintf(str_info, 1000, fmt, arg_ptr);
        va_end(arg_ptr);
        LOG_ERROR("%s", str_info);
    }
    throw result;
}

inline std::vector<std::string> split(const std::string& src,
        const std::string& delimiter) {
    std::vector<std::string> strs;
    int delimiter_len = delimiter.size();

    int index = -1, last_pos = 0;
    while (-1 != (index = src.find(delimiter, last_pos))) {
        strs.push_back(src.substr(last_pos, index - last_pos));
        last_pos = index + delimiter_len;
    }
    std::string last_str = src.substr(last_pos);
    if (!last_str.empty()) {
        strs.push_back(last_str);
    }

    return strs;
}

inline uint64_t HashId(std::string id) {
    uint64_t hash = atoll(id.c_str());
    return (hash != 0) ? hash : util::Func::BytesHash64(id.data(), id.size());
}

inline std::wstring utf8_to_wstring(const std::string& str) {
    return boost::locale::conv::utf_to_utf<wchar_t>(str.c_str(),
            str.c_str() + str.size());
}

inline std::string wstring_to_utf8(const std::wstring& str) {
    return boost::locale::conv::utf_to_utf<char>(str.c_str(),
            str.c_str() + str.size());
}

inline std::string filter_pun(const std::string& str) {
    std::locale loc("en_US.utf8");
    std::wstring ws = utf8_to_wstring(str);
    std::wstring result;
    for (size_t i = 0; i < ws.size(); i++) {
        if (!std::isalpha(ws[i], loc) && !std::isdigit(ws[i])) {
            if (i > 0) {
                result += utf8_to_wstring(" ");
            }
        } else {
            result += ws[i];
        }
    }

    return wstring_to_utf8(result);
}

}
}

#endif /* SRC_SCORING_COMMON_COMMON_H_ */
