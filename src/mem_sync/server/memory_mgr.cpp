/*
 *  memory_mgr.cpp
 *
 *  Created on: 2016-10-21
 */

#include <pthread.h>
#include "util/log.h"
#include "util/shm.h"
#include "memory_mgr.h"

namespace poseidon {
namespace mem_sync {
namespace server {

using poseidon::util::shm::ShmAttach;
using poseidon::util::shm::ShmDetach;
using poseidon::util::shm::ShmGetSize;

/* meta data return from zookeeper */
MemoryMgr::MemoryBlock::MemoryBlock()
{
	this->ref = 0;
	this->key = 0;
	this->ptr = 0;
	this->size = 0;
}

MemoryMgr::MemoryBlock::MemoryBlock(int key, int64_t size)
{
	/* just attach not create */
	LOG_INFO("get shm: key = %d, size = %d", key, size);
	this->ref = 0;
	this->key = key;
	this->ptr = NULL;
	this->size = size;
}

int MemoryMgr::MemoryBlock::Attach(void)
{
	ptr = (char *)ShmAttach(key, 0);
	if (!ptr) {
		LOG_ERROR("Attach Shared Memory failed! key = %d", key);
		size = 0;
		return -1;
	}
	return 0;
}

int MemoryMgr::MemoryBlock::Detach(void)
{
	int res = 0;
	if (ptr) {
		res = ShmDetach(ptr);
		ptr = 0;
	}
	return res;
}

MemoryMgr::MemoryBlock::~MemoryBlock()
{
	/* do nothing */
}

MemoryMgr::MemoryMgr()
{

}

MemoryMgr::~MemoryMgr()
{
	zkcli_.disconnect();
	pthread_mutex_destroy(&lock_);
	pthread_cond_destroy(&cond_);
	pthread_mutex_destroy(&map_lock_);
	Destroy();
}

void MemoryMgr::Destroy(void)
{
	DataMap::iterator data_it;
	VersionMap::iterator ver_it;

	for (data_it = mmap_.begin(); data_it != mmap_.end(); data_it++) {
		for (ver_it = data_it->second.begin(); ver_it != data_it->second.end(); ver_it++) {
			ver_it->second.Detach();
		}
	}
}

/* In the beginning of the initialization, we should wait until the
 * MemoryManager fetch the meta data from Zookeeper before accepting
 * connections. use condition for notification */
int MemoryMgr::Init(const std::string &iplist, const std::string &host, int port)
{
	zklist_ = iplist;
	host_ = host;
	port_ = port;

	pthread_mutex_init(&map_lock_, NULL);
	pthread_mutex_init(&lock_, NULL);
	pthread_cond_init(&cond_, NULL);

	pthread_mutex_lock(&lock_);
	/* zkcli Init calls zk4cpp::init */
	zkcli_.Init();
	zkcli_.connect();
	pthread_cond_wait(&cond_, &lock_);
	pthread_mutex_unlock(&lock_);
	return 0;
}

const char * MemoryMgr::GetData(int data_id, int version, int64_t offset, int64_t &size)
{
	pthread_mutex_lock(&map_lock_);
	DataMap::iterator data_it = mmap_.find(data_id);
	if (data_it == mmap_.end()) {
		LOG_INFO("shm not found(NO DATA ID): id = %d, version = %d", data_id, version);
		pthread_mutex_unlock(&map_lock_);
		return NULL;
	}

	VersionMap::iterator ver_it = data_it->second.find(version);
	if (ver_it == data_it->second.end()) {
		LOG_INFO("shm not found(NO VERSION ID): id = %d, version = %d", data_id, version);
		pthread_mutex_unlock(&map_lock_);
		return NULL;
	}

	size = ver_it->second.size;
	const char * ptr = ver_it->second.ptr;
	ver_it->second.ref++;
	pthread_mutex_unlock(&map_lock_);
	return ptr;
}

void MemoryMgr::ReleaseData(int data_id, int version)
{
	pthread_mutex_lock(&map_lock_);
	DataMap::iterator data_it = mmap_.find(data_id);
	if (data_it == mmap_.end()) {
		LOG_INFO("shm not found(NO DATA ID): id = %d, version = %d", data_id, version);
		pthread_mutex_unlock(&map_lock_);
		return;
	}

	VersionMap::iterator ver_it = data_it->second.find(version);
	if (ver_it == data_it->second.end()) {
		LOG_INFO("shm not found(NO VERSION ID): id = %d, version = %d", data_id, version);
		pthread_mutex_unlock(&map_lock_);
		return;
	}

	ver_it->second.ref--;
	pthread_mutex_unlock(&map_lock_);
}

void MemoryMgr::UpdateData(DataMap &mmap)
{
	pthread_mutex_lock(&map_lock_);
	DataMap::iterator data_it;
	for (data_it = mmap.begin(); data_it != mmap.end(); data_it++) {
		VersionMap::iterator ver_it;
		for (ver_it = data_it->second.begin(); ver_it != data_it->second.end(); ver_it++) {
			MemoryBlock &block = mmap_[data_it->first][ver_it->first];
			if (!block.ptr) {
				block.key = ver_it->second.key;
				block.size = ver_it->second.size;
				block.ref = 0;

				LOG_INFO("attach memory: dataid = %d, version = %d, "
					"key = %d, size = %d", data_it->first, ver_it->first, block.key, block.size);
				block.Attach();
			}
		}
	}
	pthread_mutex_unlock(&map_lock_);
}

void MemoryMgr::PurgeData(void)
{
	pthread_mutex_lock(&map_lock_);
	DataMap::iterator data_it;
	for (data_it = mmap_.begin(); data_it != mmap_.end(); data_it++) {
		while (data_it->second.size() > 3) {
			VersionMap::iterator ver_it = data_it->second.begin();
			if (!ver_it->second.ref) {
				if (ver_it->second.ptr)
					ver_it->second.Detach();

				LOG_INFO("purge dataid: %d, version: %d", data_it->first, ver_it->first);
				data_it->second.erase(ver_it->first);
			} else {
				break; // ptr in used
			}
		}
	}
	pthread_mutex_unlock(&map_lock_);
}

}}}


