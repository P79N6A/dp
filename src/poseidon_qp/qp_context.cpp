#include "qp_context.h"
#include "qp_log_printer.h"

namespace poseidon {
namespace qp {

QpContext::QpContext() {
    serial_num = 0;
    timeout_event = NULL;
    code = common::ERROR_NONE;
    is_timeout = false;
    has_tags = false;
    timer.start();
    send_size = 0;
    req_size = 0;
    rsp_size = 0;
    got_local_tags = false;
    need_redis_tags = false;
    got_redis_tags = false;
}
QpContext::~QpContext() {
    if (timeout_event != NULL) {
        delete timeout_event;
    }

}
void QpContext::add_tag_value(int tag_no, int tag_value) {
    string tag_value_str = util::Func::to_str(tag_value);
    add_tag_value(tag_no, tag_value_str);
}
void QpContext::add_tag_value(int tag_no, string tag_value) {
    boost::unordered_map<int, vector<string> >::iterator iter =
            qp_tag_infos.find(tag_no);
    if (iter == qp_tag_infos.end()) {
        vector < string > tag_values;
        tag_values.push_back(tag_value);
        qp_tag_infos[tag_no] = tag_values;
    } else {
        iter->second.push_back(tag_value);
    }
    has_tags = true;
}

void QpContext::interval() {
    timer.interval();
}

void QpContext::interval(const string &mark) {
    log_stream << mark << "=" << timer.interval() << "`";
}

void QpContext::stop() {
    log_stream << "use_time=" << timer.stop() << "`";
    log_stream << "id=" << id << "`";
    log_stream << "imei=" << imei << "`";
    if (imei_md5.length() > 0)
        log_stream << "imei_md5=" << imei_md5 << "`";
    log_stream << "is_timeout=" << is_timeout << "`";
    log_stream << "has_tags=" << has_tags << "`";
    log_stream << "send_size=" << send_size << "`";
    log_stream << "req_size=" << req_size << "`";
    log_stream << "rsp_size=" << rsp_size << "`";
    log_stream << "code=" << code << "`";

    LogInfo log_info;
    log_info.set_log(log_stream.str());
    LogPrinter::get_mutable_instance().push_log(log_info);
}

}
}
