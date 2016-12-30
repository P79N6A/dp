/*
 * tcpserver.h
 * Created on: 2016-11-21
 */

#ifndef CO2_SERVER_TCPSERVER_H_
#define CO2_SERVER_TCPSERVER_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <st/st.h>
#include <string>
#include <deque>

#include "co2/buffer/buffer.h"
#include "co2/stream/stream.h"

namespace co2 {
namespace server {

using co2::buffer::Pool;

class TCPServer {
public:
    TCPServer(int stack_size = 256 * 1024, int buf_size = 8192,
        int max_buff_size = 1024 * 1024);

    virtual ~TCPServer(void);

	/* create the accept socket, and bind it with the given
	 * address and port.
	 */
    int Bind(const std::string &addr, int port, int backlog = 512);

	/* create the unix domain socket,
	 */
    int BindUnix(const std::string &path, int backlog = 512);

	/* start accepting new clients, any new connection
	 * will be run in the context of a new stack.
	 */
    int Start(void);

	/* run this server until the framework stops. only
	 * one server should call this method.
	 */
    int ServeForever(void);

	/* stop accepting new connections, but the
	 * coroutines that serving the client will stop
	 * until they naturally complete.
	 */
	int Stop(void);

	/* accept callback, override this function
	 * to do what you want. don't delete the stream after
	 * using it.
	 */
	virtual int OnAccept(co2::stream::Stream &stream);

    /* use by coroutines, user should not call these functions */
	std::deque<int> &GetClients(void) { return clients_; }
    int GetSocket(void) { return accept_fd_; }
    int GetStackSize(void) { return stack_size_; }
    int GetBufSize(void) { return buf_size_; }
    int GetMaxBufSize(void) { return max_buf_size_; }
    bool IsStop(void) { return !running_; }
    bool IsUnixSocket(void) { return unix_socket_; }
    Pool *GetPool(void) { return pool_; }

    co2::stream::Stream * CreateStream(int fd);

protected:
    TCPServer(const TCPServer &server) = delete;
    TCPServer(TCPServer &&server) = delete;
    TCPServer &operator=(const TCPServer &) = delete;
    TCPServer &operator=(TCPServer &&) = delete;

protected:
    Pool *pool_;
	int accept_fd_;
	int buf_size_; /* init buffer size */
	int max_buf_size_;
	int stack_size_;
	std::deque<int> clients_;
	st_thread_t server_thread_;
	st_cond_t stop_event_;
    bool running_;
    bool unix_socket_;
};

}  // namespace server
}  // namespace co2

#endif /* CO2_SERVER_TCPSERVER_H_ */
