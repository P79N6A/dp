/*
 * Libco2: A light weigh asynchronous network framework
 *         implemented using state-thread.
 * co2.h
 * Created on: 2016-11-21
 */

#ifndef CO2_H_
#define CO2_H_

#include <st/st.h>

namespace co2 {

/* co2's error code */
enum ERROR_CODE {
    kUnknownErr = -1,
    kOK = 0,
    kReadErr,
    kWriteErr,
    kValueErr,
    kSocketErr,
    kConnectErr,
    kClosedErr,
    kListenErr,
    kUnlinkErr,
    kBindErr,
    kMemoryErr,
    kTimeoutErr,
    kNotImplementErr,
};

/* co2 log, for internal used only */
enum LOG_LEVEL {
    DEBUG,
    INFO,
    NOTICE,
    WARN,
    WARNING = WARN,
    ERR,
    ERROR = ERR,
    CRIT,
    FATAL
};

/* Init coroutine library, every main function should
 * call this before using any of the coroutine library
 * function. */
int Init(void);


int Log(int level, const char *fmt, ...);

}  // namespace cevent

#endif /* CO2_H_ */
