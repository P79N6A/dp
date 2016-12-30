#ifndef SRC_ORS_OFFLINE_CXR_STATS_UTILS_H_
#define SRC_ORS_OFFLINE_CXR_STATS_UTILS_H_

#include <sstream>
#include "protocol/src/poseidon_proto.h"
#include "util/proto_helper.h"

namespace poseidon {
namespace ors_offline {

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


inline void AlertSMS(CXRStatsConfig& config, std::string message) {
    using namespace std;
    for (int i = 0; i < config.alert_phone_number_size(); i++) {
        stringstream ss_cmd;
        ss_cmd
                << "curl -X POST -H \"Content-Type: application/json\" -d '{\"content\": \""
                << message << "\", \"address\": \""
                << config.alert_phone_number(i)
                << "\"}' 'http://api.stat.ucgc.local:8080/api/v2/util/message/sms'";
        system(ss_cmd.str().c_str());
    }
}

inline std::string GetYmdDaysBefore(int days) {
    time_t t = time(0);
    t -= 3600 * 24 * days;
    char day[9];
    strftime(day, 9, "%Y%m%d", localtime(&t));
    return std::string(day);
}

}
}

#endif /* SRC_ORS_OFFLINE_CXR_STATS_UTILS_H_ */
