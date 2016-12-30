#pragma once

#include "qp_inc.h"
#include "poseidon_qp.pb.h"

namespace poseidon {
namespace qp {

struct QpContext {
    QpContext();
    virtual ~QpContext();
    void add_tag_value(int tag_no, int tag_value);
    void add_tag_value(int tag_no, string tag_value);
    void interval();
    void interval(const string &mark);
    void stop();

    uint64 serial_num;
    string id;
    string imei;
    string imei_md5;
    common::ErrorCode code;
    bool is_timeout;
    bool has_tags;
    struct sockaddr_in peer_addr;
    QPRequest qp_request;
    QPResponse qp_response;
    util::Timer timer;
    util::TimeoutEvent *timeout_event;
    boost::unordered_map<int, vector<string> > qp_tag_infos;
    int send_size;
    int req_size;
    int rsp_size;
    stringstream log_stream;
    bool got_local_tags;
    bool got_redis_tags;
    bool need_redis_tags;
};

}
}
