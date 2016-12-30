/**
 **/
#include "log_server.h"
#include "util/log.h"
#include "util/func.h"
#include "monitor_api.h"
#include "protocol/src/poseidon_proto.h"
#include "process_log.h"
#include "process_stat.h"
#include "log_server_attr.h"
#include <unistd.h>
#include <algorithm>
#include <string>
#include <iostream>

namespace poseidon{
namespace log{

int LogServer::handle_read(const char * buf, const int len, struct sockaddr_in & client_addr)
{
    MON_ADD(ATTR_LOGSERVER_REQ, 1);
    std::string strbuf;
    strbuf.assign(buf, len);
    std::replace(strbuf.begin(), strbuf.end()-1, '\n', ' ');
    ProcessLog::get_mutable_instance().proc(strbuf.c_str(), strbuf.length());
    ProcessStat::get_mutable_instance().proc(strbuf.c_str(), strbuf.length());
    return 0;
}

}//log
}//poseidon


