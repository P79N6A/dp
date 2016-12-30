#pragma once

#include "qp_inc.h"

namespace poseidon {
namespace qp {

class QpServer: public boost::serialization::singleton<QpServer> {
public:
    int start_up();

protected:
    int _port;
    int _sock_fd;

protected:
    void process();
};

}
}
