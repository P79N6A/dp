/*
 * data_server.h
 * Created on: 2016-10-20
 */

#ifndef DATA_SERVER_H
#define DATA_SERVER_H

#include <boost/serialization/singleton.hpp>
#include "comm_event_factory.h"
#include "comm_event_tcp_listener.h"
#include "event2/event.h"

namespace poseidon {
namespace mem_sync {
namespace server {

class DataServer :
	public dc::common::comm_event::CommTcpListener,
	public boost::serialization::singleton<DataServer> {
public:
	DataServer();
	~DataServer();

	/* virtual function from tcp_listener */
	virtual int init(int fd = -1, int status = STAT_INITED);
	virtual int on_accept(int fd, sockaddr * address, int socklen);
	virtual int on_accept_error(void);
	void on_timer(void);

private:
	struct event * timer_;
};

}}}

#endif
