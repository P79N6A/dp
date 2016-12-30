/*
 * File Name: zkclient.cpp
 * Created on: 2016-10-21
 */

#include <list>
#include <vector>
#include <string>
#include <memory>
#include <boost/algorithm/string.hpp>

#include "util/log.h"
#include "util/func.h"
#include "zk_client.h"
#include "config.h"
#include "memory_mgr.h"

#define MAX_DATA 256
#define MAX_PATH 256

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

namespace poseidon {
namespace mem_sync {
namespace server {

ZKClient::ZKClient() : first_connect_(true)
{

}

/* this function should be called after mgr_ has been initialized */
void ZKClient::Init(void)
{
	using poseidon::mem_sync::Config;
	Config &conf = Config::get_mutable_instance();

	host_ = conf.LocalIP();
	port_ = conf.ServerPort();
	zklist_ = conf.ZKList();

	init(zklist_);
}

/* this function is called in the thread managed by Zookeeper's lib */
void ZKClient::onConnect(void)
{
	LOG_INFO("zkcli begin init");
	UpdateData();

	MemoryMgr &mgr = MemoryMgr::get_mutable_instance();
	LOG_INFO("zkcli init done");

	if (first_connect_) {
		/* notify the main thread we're done here. */
		first_connect_ = false;
		pthread_mutex_lock(&mgr.lock_);
		pthread_cond_signal(&mgr.cond_);
		pthread_mutex_unlock(&mgr.lock_);
	}
}

void ZKClient::onExpired(void)
{
	LOG_WARN("ZKConnection lost!");
	dataids_.clear();
	connect();
}

void ZKClient::onNodeChildChanged(int state, const char * path)
{
	/* For simplicity what ever changed we call the UpdateData to build *ALL*,
	 * note that optimization does exist. see UpdateData for detail. */
	LOG_INFO("onNodeChildChanged: path = %s", path);
	UpdateData();
}

void ZKClient::onNodeChanged(int state, const char * path)
{
	LOG_INFO("onNodeChanged: path = %s", path);

	std::vector<std::string> vec;
	boost::split(vec, path, boost::is_any_of("/"));

	/* we only need to handle new version:
	 * e.g. ('', mem_sync, $dataid, config, version)
	 */
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
		return UpdateData(dataid, version);
	}
}

/* @Brief: Get how many dataids altogether, then for each dataid
 * we *TRY* to get the config
 */
void ZKClient::UpdateData(void)
{
	std::list<std::string> ids; /* data id */
	std::list<std::string>::iterator iit;

	int res = getNodeChildren("/", ids, 1);
	if (res < 1) {
		return;
	}

	res = getNodeChildren("/mem_sync", ids, 1);
	LOG_INFO("getNodeChildren /mem_sync: %d", res);
	if (res < 1) {
		return;
	}

	for (iit = ids.begin(); iit != ids.end(); iit++) {
		int data_id = atoi((*iit).c_str());

		/* skip those we have already watch */
		if (dataids_.find(data_id) != dataids_.end())
			continue;

		UpdateData(data_id);
	}
}

void ZKClient::UpdateData(int data_id)
{
	char data[MAX_DATA];
	char path[MAX_PATH];
	uint32_t len;
	Stat stat;
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
	for (it = list.begin(); it != list.end(); it++) {
		LOG_INFO("find node: /mem_sync/%d/%s", data_id, (*it).c_str());
		if (*it == "config") {
			find_config  = true;
			break;
		}
	}

	if (find_config) {
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
			UpdateData(data_id, version);
		}
	}
}

void ZKClient::UpdateData(int data_id, int version)
{
	char data[MAX_DATA];
	char path[MAX_PATH];
	uint32_t len;
	Stat stat;
	int res;

	sprintf(path, "/mem_sync/%d/%d/data_server_addr", data_id, version);
	len = sizeof(data);
	res = getNodeData(path, data, len, stat, 0);
	if (res < 1) {
		LOG_INFO("get data_server_addr failed: %d, %s", res, path);
		return;
	}

	std::string host;
	int port = 0;
	GetHostPort(data, host, port);
	LOG_INFO("host = %s, port = %d, host_ = %s, port_ = %d",
		host.c_str(), port, host_.c_str(), port_);

	if (host != host_ || port != port_) {
		return;
	}

	sprintf(path, "/mem_sync/%d/%d/shm_key", data_id, version);
	len = sizeof(data);
	res = getNodeData(path, data, len, stat, 0);
	if (res < 1) {
		LOG_INFO("get shm_key failed: %d, %s", res, path);
		return;
	}
	int shm_key = atoi(data);

	sprintf(path, "/mem_sync/%d/%d/shm_size", data_id, version);
	len = sizeof(data);
	res = getNodeData(path, data, len, stat, 0);
	if (res < 1) {
		LOG_INFO("get shm_key failed: %d, %s", res, path);
		return;
	}
	int64_t shm_size = strtoull(data, NULL, 10);

	LOG_INFO("add to map: data_id = %d, version_id = %d, "
		"shm_key = %d, shm_size = %d", data_id, version, shm_key, shm_size);

	MemoryMgr::DataMap mmap;
	MemoryMgr::MemoryBlock block(shm_key, shm_size);
	mmap[data_id][version] = block;

	MemoryMgr &mgr = MemoryMgr::get_mutable_instance();
	mgr.UpdateData(mmap);
}


}}}
