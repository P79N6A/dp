#pragma once

#include "logcheck_inc.h"
#include "pvlog_rules.h"
#include "os_rules.h"

namespace poseidon {
namespace log_check {

class LogCheckWorker {
public:
    LogCheckWorker() {
        _base = NULL;
        _serial_num = 0;
    }
    virtual ~LogCheckWorker() {
        if (_base != NULL) {
            event_base_free(_base);
        }
    }
    void start_up(int sock_fd);
    void process(char * recv_buf, int recv_len, const struct sockaddr_in &addr);

protected:
    struct event_base* _base;
    int _sock_fd;
    uint64_t _serial_num;
    PvLogRules pv_rules_;
    OsRules os_rules_;

protected:

};

}
}
