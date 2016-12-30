/*
 * zk_client.cpp
 * Created on: 2016-10-25
 */

#include "zk_client.h"

#include <errno.h>
#include <string.h>
#include <sys/ipc.h>
#include <list>
#include <string>
#include <boost/algorithm/string.hpp>

#include "common/comm_macro.h"
#include "common/proto.h"
#include "util/log.h"
#include "util/func.h"
#include "util/ipc_mq.h"

#include "data_agent.h"
#include "config.h"
#include "monitor_api.h"
#include "agent_attr.h"

#define MAX_PATH 256
#define MAX_DATA 256

using poseidon::mem_sync::agent::Config;
typedef poseidon::monitor::Api MonitorApi;

namespace poseidon {
namespace mem_sync {
namespace agent {

/* @brief: get host and port from string host:port,
 * @return: 0 if success, otherwise fail */
static int GetHostPort(const char * host_port, std::string &host, int &port)
{
	const char * pos = strchr(host_port, ':');
	if (pos == NULL) {
		return -1;
	}

	host.assign(host_port, pos - host_port);
	port = atoi(pos + 1);
	// LOG_INFO("host = %s, port = %d", host.c_str(), port);
	return 0;
}

ZKClient::ZKClient()
{
	using poseidon::mem_sync::agent::Config;
	Config &conf = Config::get_mutable_instance();

	host_ = conf.LocalIP();
}

int ZKClient::Init(const std::string &zklist)
{
    /* init mq */
    if (0 != mq_.init(IPC_MQ_KEY,  IPC_CREAT | 0666)) {
        LOG_ERROR("Create MQ failed: %s", strerror(errno));
        return -1;
    }

    return init(zklist);
}

ZKClient::~ZKClient()
{

}

/* get default config from ZK:
 * now the tree in zk is as follows:
 * mem_sync -> dataid(1) -> config --> version
 *                 |
 *                 |
 *                 `------> agents
 *                 |           |
 *                 |           `--- ip1(json)
 *                 |           |
 *                 |           `----ip2(json)
 *                 |
 *                 `------> version(N)
 */

void ZKClient::onConnect(void)
{
	/* report when onConnect or reConnect */
	MonitorApi &mon = MonitorApi::get_mutable_instance();
	mon.mon_add(ATTR_DATA_AGENT_ZK_CONNECT, 1);

	LOG_INFO("ZKConnected");

	dataids_.clear();
	// Report Version is implemented in dataagent module.
	// ReportVersion();
	UpdateData();
}

void ZKClient::onExpired(void)
{
	/* report when onExpired */
	MonitorApi &mon = MonitorApi::get_mutable_instance();
	mon.mon_add(ATTR_DATA_AGENT_ZK_LOST, 1);

	LOG_WARN("ZKConnection lost!");
	connect();
}

void ZKClient::UpdateData(void)
{
	std::list<std::string> ids;
	std::list<std::string>::iterator it;

	int res = getNodeChildren("/", ids, 1);
	if (res < 1) {
		return;
	}

	res = getNodeChildren("/mem_sync", ids, 1);
	if (res < 1) {
		LOG_ERROR("GetChildren /mem_sync Failed");
		return;
	}

	for (it = ids.begin(); it != ids.end(); it++) {
		int data_id = atoi((*it).c_str());
		UpdateData(data_id);
	}
}

/* @brief: we found ${data_id} on zookeeper, and we want to
 *     monitor its version to see if it is updated
 */
void ZKClient::UpdateData(int data_id)
{
	char path[MAX_PATH];
	sprintf(path, "/mem_sync/%d", data_id);

	std::list<std::string> list;
	std::list<std::string>::iterator it;

	int res = getNodeChildren(path, list, 1);
	if (res < 1) {
		LOG_WARN("Get info from dataid failed: data_id = %d, "
			"path = %s", data_id, path);
		return;
	}

	bool find_config = false;
	bool find_agents = false;

	for (it = list.begin(); it != list.end(); it++) {
		LOG_INFO("find node: /mem_sync/%d/%s", data_id, (*it).c_str());
		if (*it == "config") {
			find_config = true;
		} else if (*it == "agents") {
			find_agents = true;
		}
	}

	if (find_config) {
		/* only cares those not monitored */
		if (dataids_.find(data_id) == dataids_.end()) {
			UpdateConfig(data_id);
		}
	}
	if (find_agents) {
		NotifyAgent(data_id);
	}
}

/* @brief: check to see if agent needs this ${data_id},
 * if is, send it to the message queue to notify agent. */
void ZKClient::NotifyAgent(int data_id)
{
	char path[MAX_PATH];
	sprintf(path, "/mem_sync/%d/agents", data_id);

	std::list<std::string> list;
	std::list<std::string>::iterator it;

	DataAgent &agent = DataAgent::get_mutable_instance();

	int res = getNodeChildren(path, list, 1);
	if (res < 1) {
		LOG_WARN("Get info from dataid failed: data_id = %d, "
			"path = %s", data_id, path);
		return;
	}

	for (it = list.begin(); it != list.end(); it++) {
		LOG_INFO("find node: /mem_sync/%d/agents/%s", data_id, (*it).c_str());
		if (*it == host_) {
			LOG_INFO("Add dataid(%d) to monitor", data_id);

		    /* let agent know it needs this dataid. */
		    mq_.push(MQ_TYPE_API, (void *)&data_id, sizeof(int));
		}
	}
}

void ZKClient::UpdateConfig(int data_id)
{
	char path[MAX_PATH];
	char data[MAX_DATA];

	std::list<std::string> list;
	std::list<std::string>::iterator it;
	int res;
	Stat stat;
	uint32_t len;

	sprintf(path, "/mem_sync/%d/config", data_id);
	list.clear();
	res = getNodeChildren(path, list, 1);
	if (res < 1)
		return; /* version node *NOT* ready */

	bool find_version = false;
	for (it = list.begin(); it != list.end(); it++) {
		LOG_INFO("find node: /mem_sync/%d/config/%s", data_id, (*it).c_str());
		if (*it == "version") {
			find_version = true;
		}
	}
	if (find_version) {
		sprintf(path, "/mem_sync/%d/config/version", data_id);
		len = sizeof(data);

		res = getNodeData(path, data, len, stat, 1);
		if (res < 1) {
			LOG_WARN("Get version from config failed: data_id = %d, "
				"path = %s", data_id, path);
			return;
		}
		dataids_.insert(data_id);
		int version = atoi(data);
		UpdateVersion(data_id, version);
	}
}

void ZKClient::UpdateVersion(int data_id, int version)
{
	DataAgent &agent = DataAgent::get_mutable_instance();
	DataAgent::StatMap::iterator it;

	char path[MAX_PATH];
	char data[MAX_PATH];
	uint32_t len;
	Stat stat;

	/* Get Host and Port */
	sprintf(path, "/mem_sync/%d/%d/data_server_addr", data_id, version);
	len = sizeof(data);
	int res = getNodeData(path, data, len, stat, 0);
	if (res < 1) {
		LOG_INFO("get data_server_addr failed: %d, %s", res, path);
		return;
	}

	std::string host;
	int port = 0;
	GetHostPort(data, host, port);

	/* Get Share Memory Key, temporarily not used */
	sprintf(path, "/mem_sync/%d/%d/shm_key", data_id, version);
	res = getNodeData(path, data, len, stat, 0);
	if (res < 1) {
		LOG_INFO("get shm_key failed: %d, %s", res, path);
		return;
	}
	// int shm_key = atoi(data);

	/* Get Share Memory Size, temporarily not used */
	sprintf(path, "/mem_sync/%d/%d/shm_size", data_id, version);
	len = sizeof(data);
	res = getNodeData(path, data, len, stat, 0);
	if (res < 1) {
		LOG_INFO("get shm_key failed: %d, %s", res, path);
		return;
	}
	// int shm_size = atoi(data);

	/* Get Checksum */
	sprintf(path, "/mem_sync/%d/%d/check_sum", data_id, version);
	len = sizeof(data);
	res = getNodeData(path, data, len, stat, 0);
	if (res < 1) {
		LOG_INFO("get shm_key failed: %d, %s", res, path);
		return;
	}
	std::string md5(data);

	/* notify agent the newest version */
	DataInfo data_info;
	data_info.data_id = data_id;
	data_info.version = version;
	data_info.host = inet_addr(host.c_str());
	data_info.port = port;
	strncpy(data_info.md5, md5.c_str(), 128);
	data_info.md5[128] = '\0';

	mq_.push(MQ_TYPE_UPDATE, &data_info, sizeof(DataInfo));
}

/* report all versions */
void ZKClient::ReportVersion(void)
{
	DataAgent &agent = DataAgent::get_mutable_instance();
	DataAgent::StatMap::iterator it;

	for (it = agent.map_.begin(); it != agent.map_.end(); it++) {
		if (it->second.stat & DataAgent::USED) {
			ReportVersion(it->first, it->second.cur_ver);
		}
	}
}

/* NOTE: this function is called in the main thread, we should not
 * call the blocking method of zookeeper client. The principle is
 * (1) We should never call blocking method in the main thread.
 * (2) When the method is async and is called in the main thread,
 * we should *NOT* use lock.
 */
void ZKClient::ReportVersion(int data_id, int version)
{
	char path[MAX_PATH];
	char ver[16];
	size_t len = sizeof(ver);
	int res;

	sprintf(path, "/mem_sync/%d/agents/%s", data_id, host_.c_str());
	res = createNodeAsync(path, "", 0);
	if (res < 0) {
		LOG_WARN("Create Node failed! Path = %s", path);
		return;
	}
}

/* Now this method is called in the zookeeper's thread, so
 * blocking methods are allowed here.
 */
void ZKClient::OnCreateNodeAsync(const std::string& path, int rc)
{
	std::vector<std::string> vec;
	boost::split(vec, path, boost::is_any_of("/"));

	if (rc == -1) {
		LOG_INFO("OnCreateNodeFailed: path = %s", path.c_str());
		return;
	} else if (rc == ZNODEEXISTS) {
		/* do nothing */;
	} else if (rc == ZNONODE) {
		/* Blocking but OK */
		rc = createNode(path, "", 0, NULL, -1, true);
		if (rc < 0) {
			LOG_WARN("OnCreateNode Failed: path = %s", path.c_str());
			return;
		}
	}

	DataAgent &agent = DataAgent::get_mutable_instance();
	DataAgent::StatMap::iterator it;

	/* we monitor every /mem_sync/$data_id/agents/$ip */
	if (vec.size() == 5 && vec[3] == "agents") {
		int data_id = atoi(vec[2].c_str());
		char data[MAX_DATA];

		pthread_mutex_lock(&agent.lock_);
		std::map<int, DataAgent::MemStat>::iterator it = agent.map_.find(data_id);
		if (it == agent.map_.end()) {
			pthread_mutex_unlock(&agent.lock_);
			return;
		}

		/* temporarily set version only */
		sprintf(data, "{\"version\": %d}", it->second.cur_ver);
		pthread_mutex_unlock(&agent.lock_);

		/* Blocking, but OK. */
		setNodeData(path, data, strlen(data));
	}
}

/* refer server::zk_client for detailed information about onNodeChanged
 *
 */
void ZKClient::onNodeChanged(int state, const char * path)
{
	LOG_INFO("onNodeChanged: path = %s", path);

	std::vector<std::string> vec;
	boost::split(vec, path, boost::is_any_of("/"));

	if (vec.size() == 5 && vec[4] == "version") {
		int dataid = atoi(vec[2].c_str());
		char data[MAX_DATA];
		uint32_t len;
		Stat st;
		len = sizeof(data);
		int res = getNodeData(path, data, len, st, 1); /* re-watch this node */
		if (res < 1) {
			LOG_WARN("getNodeData failed: path = %s", path);
			return;
		}

		int version = atoi(data);
		LOG_INFO("dataid %d version %d found", dataid, version);
		return UpdateVersion(dataid, version);
	}
}

void ZKClient::onNodeChildChanged(int state, char const* path)
{
	LOG_INFO("onNodeChildChanged: %s", path);

	std::vector<std::string> vec;
	boost::split(vec, path, boost::is_any_of("/"));

	/* we monitor every /mem_sync/$data_id/agents */
	if (vec.size() == 4) {
		int data_id = atoi(vec[2].c_str());
		if (vec[3] == "agents") {
			NotifyAgent(data_id);
		} else if (vec[3] == "config") {
			UpdateConfig(data_id);
		}
	} else if (vec.size() == 3) {
		int data_id = atoi(vec[2].c_str());
		UpdateData(data_id);
	} else {
		/* Root Change or /mem_sync changed, probably */
		UpdateData();
	}
}

}}}


