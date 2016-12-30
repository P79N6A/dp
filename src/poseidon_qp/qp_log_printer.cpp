#include "qp_log_printer.h"

namespace poseidon {
namespace qp {

void LogPrinter::startup() {
    util::Closure *run = util::NewCallback(this, &LogPrinter::run);
    util::ThreadPool::get_mutable_instance().add_task(run);
}

void LogPrinter::run() {
    while (1) {
        LogInfo log_info;
        while (_log_str_queue.pop(log_info)) {
            LOG_NOTICE("%s", log_info.get_log());
            log_info.destroy();
        }
        usleep(1000 * 10);
    }
}

void LogPrinter::push_log(const LogInfo & log_info) {
    _log_str_queue.push(log_info);
}
}
}
