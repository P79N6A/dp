/*
 * data_server.cpp
 * Created on: 2016-10-20
 */

#include "data_client.h"
#include "data_server.h"
#include "memory_mgr.h"

#include "util/log.h"
#include "util/func.h"

namespace poseidon {
namespace mem_sync {
namespace server {

DataServer::DataServer() : timer_(NULL)
{

}

DataServer::~DataServer()
{
	if (timer_) {
		event_del(timer_);
		event_free(timer_);
	}
}

static void TimerCallback(evutil_socket_t fd, short event, void * arg)
{
	// LOG_INFO("on timer");
	DataServer * server = (DataServer *)arg;
	server->on_timer();
}

void DataServer::on_timer(void)
{
	MemoryMgr & mgr = MemoryMgr::get_mutable_instance();
	mgr.PurgeData();
}

int DataServer::init(int fd, int status)
{
	int res = dc::common::comm_event::CommTcpListener::init(fd, status);
	if (res != 0) {
		LOG_ERROR("Init DataServer failed");
		return res;
	}

	/* init timer event */
	timer_ = event_new(base_, -1, EV_PERSIST, TimerCallback, (void *)this);
	if (timer_ == NULL) {
		LOG_WARN("Can not create timer event");
		delete this;
		return -1;
	}

	struct timeval tv;
	tv.tv_sec = 10; /* 10 secs */
	tv.tv_usec = 0;
	evtimer_add(timer_, &tv);
	return 0;
}

int DataServer::on_accept(int fd, sockaddr * address, int socklen)
{
	LOG_DEBUG("on_accept called, client[%s]",
		util::Func::to_str(*(struct sockaddr_in *)address).c_str());

	DataClient * conn = new(std::nothrow) DataClient;
    if (!conn) {
		LOG_WARN("create DataClient failed");	
		return -1;
	}

	/* set event_base in the AgentClient by calling add_comm_tcp */
	dc::common::comm_event::CommFactoryInterface::instance().add_comm_tcp(conn);

	conn->init(fd, STAT_CONNECTED);
	return 0;
}

int DataServer::on_accept_error(void)
{
    LOG_DEBUG("%s called\n", __FUNCTION__);
    return 0;
}

}}}
