#pragma once

#include "logcheck_inc.h"

namespace poseidon {
namespace log_check {

class LogCheckServer: public boost::serialization::singleton<LogCheckServer> {
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
