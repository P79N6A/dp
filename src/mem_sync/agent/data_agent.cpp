/*
 * data_agent.cpp
 * Created on: 2016-10-24
 */
#include "data_agent.h"

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include "config.h"
#include "util/log.h"
#include "util/ipc_mq.h"
#include "zk_client.h"
#include "agent_client.h"
#include "common/comm_macro.h"
#include "storage.h"
#include "monitor_api.h"
#include "agent_attr.h"

using poseidon::mem_sync::agent::Storage;

namespace poseidon {
namespace mem_sync {
namespace agent {

static void TimerCallback(evutil_socket_t fd, short ev, void * arg)
{
	DataAgent * agent = (DataAgent *)arg;
	agent->on_timer();
}

DataAgent::DataAgent(void) : timer_(NULL), msgid_(0)
{

}

DataAgent::~DataAgent(void)
{
	if (timer_) {
		event_del(timer_);
		event_free(timer_);
	}

	zkcli_.disconnect();
	pthread_mutex_destroy(&lock_);
}

int DataAgent::init(int fd, int status)
{
	int res = -1;
	pthread_mutex_init(&lock_, NULL);

	Config &config = Config::get_mutable_instance();

	do {
		timer_ = event_new(base_, -1, EV_PERSIST, TimerCallback, this);
		if (!timer_) {
			LOG_ERROR("Create Timer failed");
			break;
		}
		struct timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		event_add(timer_, &tv);

		if (0 != CommTcpListener::init(fd, status)) {
			LOG_ERROR("DataAgent init failed!");
			break;
		}

		if (0 != msgq_.init(IPC_MQ_KEY,  IPC_CREAT | 0666)) {
			LOG_ERROR("Create MQ failed: %s", strerror(errno));
			break;
		}

		/* init mq then zklist */
		if (0 != zkcli_.Init(config.ZKList())) {
			LOG_ERROR("Init ZKCli failed!");
			break;
		}
		zkcli_.connect();
		res = 0;
	} while (0);
	return res;
}

void DataAgent::InitDataIDs(const std::map<int, int> &ids)
{
	map_.clear();

	std::map<int, int>::const_iterator it;
	for (it = ids.begin(); it != ids.end(); it++) {
		MemStat &st = map_[it->first];
		st.cur_ver = it->second;
		st.new_ver = it->second;
		st.host = "";
		st.md5 = "";
		st.port = 0;
		st.stat = USED;
	}
}

void DataAgent::on_timer(void)
{
	long type;
	int size;
	int res;
	int data_id;
	char data[8192];

	std::set<int> s;
	std::set<int>::iterator it;
	std::map<int, MemStat>::iterator mit;

	while (true) {
		res = msgq_.get(0, /* api's request: 15 */
			type,
			data,
			size,
			MSG_NOERROR | IPC_NOWAIT);

		if (res == -1) {
			if (errno != EAGAIN && errno != ENOMSG && errno != EIDRM) {
				LOG_WARN("Get Message(type = 1) failed(%d): %s!",
					errno, strerror(errno));
			}
			break;
		} else {
		    LOG_INFO("get message: type = %d, size = %d", type, size);
		}

		if (type == MQ_TYPE_API) {
			data_id = *(int *)data;
			if (data_id == 0) { /* test */
				DumpMemStat();
				continue;
			}

			s.insert(data_id);
			LOG_INFO("get dataid from MQ: %d, type = %d, size = %d",
					data_id, type, size);

		} else if (type == MQ_TYPE_UPDATE) {
		    /* update from zookeeper */
		    DataInfo * di = (DataInfo *)data;
		    MemStat &st = map_[di->data_id];
		    if (st.new_ver < di->version) {
		        st.new_ver = di->version;
		        st.md5 = std::string(di->md5);
		        st.port = di->port;

		        struct in_addr in_addr;
		        in_addr.s_addr = di->host;
		        st.host = inet_ntoa(in_addr);
		        st.stat &= ~DataAgent::BAD;
		        st.stat |= DataAgent::PENDING;
		    }
		    LOG_INFO("Update DataInfo: dataid: %d, ver: %d, "
		        "md5: %s, host: %s, port: %d", di->data_id, st.new_ver,
		        st.md5.c_str(), st.host.c_str(), st.port);
		}
	}

	for (it = s.begin(); it != s.end(); it++) {
		UpdateData(*it, true);
	}

	for (mit = map_.begin(); mit != map_.end(); mit++) {
		UpdateData(mit->first, false);
	}
}

/* @return: if a socket is created successfully and connecting
 * to server 0 is return, otherwise nonzero is return.
 */
int DataAgent::UpdateData(int data_id, bool add)
{
	StatMap::iterator it = map_.find(data_id);
	if (it == map_.end()) {
		LOG_INFO("create dataid: %d", data_id);

		MemStat &st = map_[data_id];
		st.stat |= (USED | PENDING);
	}

	it = map_.find(data_id);
	if (add) {
		it->second.stat |= USED;
	}

	if (!(it->second.stat & USED)) {
	    return -1;
	}

	if (it->second.cur_ver < it->second.new_ver) {
	    /* may be due to updating, but should not always happen */
	    LOG_INFO("version not match: current(%d), new(%d)",
	        it->second.cur_ver, it->second.new_ver);

	    poseidon::monitor::Api &api =
	        poseidon::monitor::Api::get_mutable_instance();
	    api.mon_add(ATTR_DATA_AGENT_VER_NOT_MATCH, 1);
	}

	if ((it->second.stat & BAD) || (it->second.host.empty())) {
	    /* BAD VERSION, due to memmgr update failed or md5 error */
		return -1;
	}

	/* we have already got host from zk, fetch this memory block */
	if ((it->second.stat & PENDING) && !(it->second.stat & UPDATING)) {
		if (it->second.cur_ver == it->second.new_ver) {
			LOG_INFO("current version: %d == new version %d ",
				it->second.cur_ver, it->second.new_ver);

			it->second.stat &= ~PENDING;
			return -1;
		}

		/* check address first before create client */
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(it->second.host.c_str());
        addr.sin_port = htons(it->second.port);

        if (addr.sin_addr.s_addr == INADDR_NONE) {
            LOG_ERROR("addr %s not valid", it->second.host.c_str());
            return -1;
        }

		dc::common::comm_event::CommFactoryInterface &factory =
			dc::common::comm_event::CommFactoryInterface::instance();

		AgentClient * client = new (std::nothrow)AgentClient(data_id,
			it->second.new_ver, it->second.md5);

		if (!client) {
			LOG_ERROR("Create AgentClient failed!");
			return -1;
		} else {
		    LOG_INFO("Create AgentClient %p", client);
		}

        factory.add_comm_tcp(client);
        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        evutil_make_socket_nonblocking(sock);
        client->init(sock, STAT_INITED);

		if (0 != client->connect(addr)) {
		    /* if connect failed, on error will be called
		     * so we don't need to delete client here */
			LOG_ERROR("Connect to server[%s:%d] failed!",
			    it->second.host.c_str(), it->second.port);
			return -1;
		} else {
			LOG_INFO("Connecting to server[%s:%d] fetching dataid(%d), version(%d)",
				it->second.host.c_str(), it->second.port,
				data_id, it->second.new_ver);
		}

		it->second.stat &= ~PENDING;
		it->second.stat |= UPDATING;
		return 0;
	}
	return -1;
}

/* @brief: stat is bit combinations that indicate what we should update,
 * if success, stat should be 0,
 * if failed temparorily, stat should be PENDING,
 * if MD5 not equal, stat should be BAD */
int DataAgent::UpdateMemStat(int data_id, int version, int stat)
{
	StatMap::iterator it = map_.find(data_id);
	do {
		if (it == map_.end()) {
			LOG_ERROR("data_id not found: %d", data_id);
			break;
		}
		if (stat == 0) { /* success */
			it->second.cur_ver = version;

			/* tell zk that we have updated */
			LOG_INFO("Report Version: dataid = %d, version = %d", data_id, version);
			zkcli_.ReportVersion(data_id, version);

			Storage &storage = Storage::get_mutable_instance();
			storage.AddDataID(data_id, version);
			storage.DumpData();

		} else {
			it->second.stat |= stat;
		}

		it->second.stat &= ~UPDATING;
	} while (0);
	return 0;
}

/* test code */
void DataAgent::DumpMemStat(void)
{
	StatMap::iterator it;

	for (it = map_.begin(); it != map_.end(); it++) {
		LOG_INFO("dataid(%d), cur_ver(%d), new_ver(%d), stat(%d), host(%s), port(%d)",
			it->first, it->second.cur_ver, it->second.new_ver, it->second.stat,
			it->second.host.c_str(), it->second.port);
	}
}


}}}
