/*
 * udp_server.h
 * Created on: 2016-11-24
 */

#ifndef CO2_SERVER_UDP_SERVER_H_
#define CO2_SERVER_UDP_SERVER_H_

#include <netinet/in.h>
#include <st/st.h>

#include <deque>
#include <string>

#include "co2/buffer/buffer.h"

namespace co2 {
namespace server {

class UDPServer {
public:
    UDPServer(int stack_size = 64 * 1024, int packet_size = 64 * 1024);
    virtual ~UDPServer();

    /* start accepting new clients, any new connection
     * will be run in the context of a new stack. */
    int Start();

    /* run this server until the framework stops. only
     * one server should call this method. */
    int ServeForever();

    /* stop server */
    int Stop();

	/* bind the given address:port to become a server. */
    int Bind(const std::string &addr, int port);

	/* send data to the remote address. */
    int WriteBytes(const std::string &host, int port,
        const void *buf, int size);

    /* override this function to implement your logic */
    virtual int OnDatagram(const std::string &host, int port,
        const char *buf, int size);

    ///////////////////////////////////////////////////////////////////////////
    // Internal use only
    const std::string &GetAddr() const { return addr_; }
    int GetPort() { return port_; }
    int GetSocket() { return accept_fd_; }
    int GetStackSize() { return stack_size_; }
    int GetPacketSize() { return packet_size_; }
    co2::buffer::Pool *GetPool() { return pool_; }
    bool IsStop() { return !running_; }
    std::deque<co2::buffer::buffer_ *> &GetClients() { return clients_; }

protected:
    UDPServer(const UDPServer &server) = delete;
    UDPServer &operator=(const UDPServer &server) = delete;

protected:
    int accept_fd_;
    int stack_size_;
    int packet_size_; // max packet size
    std::string addr_;
    int port_;
    bool running_;
    co2::buffer::Pool *pool_;
    st_thread_t server_thread_;
    st_cond_t stop_event_;
    std::deque<co2::buffer::buffer_ *> clients_;
};

}  // namespace server
}  // namespace co2


#endif /* CO2_SERVER_UDP_SERVER_H_ */
