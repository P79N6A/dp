#include "logcheck_server.h"
#include "logcheck_worker.h"

DEFINE_int32(udp_serv_port, 53001, "UDP绑定端口");
DEFINE_int32(udp_serv_worker, 1, "pvlog_checker服务工作线程数");

namespace poseidon {
namespace log_check {

int LogCheckServer::start_up() {
    _port = FLAGS_udp_serv_port;
    if (_port == 0)
        return -1;
    _sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in serv_addr;
    socklen_t serv_addr_len;
    util::Func::gen_addr(_port, serv_addr, serv_addr_len);
    if (bind(_sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))
            == -1) {
        return -2;
    }
    evutil_make_listen_socket_reuseable(_sock_fd);
    evutil_make_socket_nonblocking(_sock_fd);
    for (int i = 0; i < FLAGS_udp_serv_worker; i++) {
        util::Closure *process = util::NewCallback(this, &LogCheckServer::process);
        util::ThreadPool::get_mutable_instance().add_task(process);
    }

    return 0;
}

void LogCheckServer::process() {
    LogCheckWorker worker;
    worker.start_up(_sock_fd);
}

}
}
