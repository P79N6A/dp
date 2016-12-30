#pragma once

#include "qp_inc.h"

namespace poseidon {
namespace qp {

class QpToolkits {
public:
    static bool gen_addr(int port, struct sockaddr_in & addr,
            socklen_t &addr_len);
    static bool gen_addr(const string &host, int port,
            struct sockaddr_in & addr, socklen_t &addr_len);
    static string get_bucket_key(const string & uid) {
        string bucket_key;
        if (uid.length() > BULKET_INDEX_LEN)
            bucket_key = uid.substr(0, uid.length() - BULKET_INDEX_LEN);
        else
            bucket_key = uid;
        return bucket_key;
    }

    static string get_bucket_index(const string & uid) {
        string bucket_index;
        if (uid.length() > BULKET_INDEX_LEN)
            bucket_index = uid.substr(uid.length() - BULKET_INDEX_LEN,
                    BULKET_INDEX_LEN);
        else
            bucket_index = uid;
        return bucket_index;
    }
};
}
}
