#include "qp_server.h"
#include "qp_toolkits.h"
#include "qp_worker.h"

DEFINE_int32(udp_serv_port, 46001, "UDP绑定端口");
DEFINE_int32(udp_serv_worker, 1, "QP服务工作线程数");

namespace poseidon {
namespace qp {

int QpServer::start_up() {
    _port = FLAGS_udp_serv_port;
    if (_port == 0)
        return -1;
    _sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in serv_addr;
    socklen_t serv_addr_len;
    QpToolkits::gen_addr(_port, serv_addr, serv_addr_len);
    if (bind(_sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))
            == -1) {
        return -2;
    }
    evutil_make_listen_socket_reuseable(_sock_fd);
    evutil_make_socket_nonblocking(_sock_fd);
    for (int i = 0; i < FLAGS_udp_serv_worker; i++) {
        util::Closure *process = util::NewCallback(this, &QpServer::process);
        util::ThreadPool::get_mutable_instance().add_task(process);
    }

    return 0;
}

void QpServer::process() {
    QpWorker worker;
    worker.start_up(_sock_fd);
}

}
}
