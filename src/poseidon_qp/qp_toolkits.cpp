#include "qp_toolkits.h"

namespace poseidon {
namespace qp {

bool QpToolkits::gen_addr(int port, struct sockaddr_in & addr,
        socklen_t &addr_len) {
    addr_len = sizeof(struct sockaddr_in);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    return true;
}
bool QpToolkits::gen_addr(const string &host, int port,
        struct sockaddr_in & addr, socklen_t &addr_len) {
    addr_len = sizeof(struct sockaddr_in);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, host.c_str(), (void *) &addr.sin_addr.s_addr);
    addr.sin_port = htons(port);
    return true;
}

}
}
