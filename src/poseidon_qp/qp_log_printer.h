#pragma once

#include "qp_inc.h"
#include "poseidon_qp.pb.h"

namespace poseidon {
namespace qp {

class LogInfo {
public:
    LogInfo() {
        buff = NULL;
    }
    void set_log(const string & log_str) {
        buff_len = log_str.length() + 1;
        buff = new char[log_str.length() + 1];
        memset(buff, 0, buff_len);
        strcpy(buff, log_str.c_str());
    }
    void destroy() {
        delete[] buff;
    }
    char * get_log() {
        if (buff != NULL)
            return buff;
    }
protected:
    char * buff;
    int buff_len;
};

class LogPrinter: public boost::serialization::singleton<LogPrinter> {
public:
    void startup();
    void push_log(const LogInfo & log_info);

protected:
    boost::lockfree::queue<LogInfo, boost::lockfree::capacity<1024 * 10> > _log_str_queue;

protected:
    void run();
};

}
}
