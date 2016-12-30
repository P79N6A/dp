/*
 * File Name: memory_mgr.h
 * Created on: 2016-10-21
 */

#ifndef MEMORY_MGR_H_
#define MEMORY_MGR_H_

#include <map>
#include <boost/serialization/singleton.hpp>

#include "util/mutex.h"
#include "zk_client.h"

using namespace std;
using namespace boost;

namespace poseidon {
namespace mem_sync {
namespace server {

/* MemoryManager manage's the memory block copy associated with the given ID,
 * It can receive update notification from the zookeeper to refresh its
 * memory block. MemoryManager will be run in a separate thread.
 */

class MemoryMgr : public boost::serialization::singleton<MemoryMgr> {
public:
	/* SharedMemory's meta */
	struct MemoryBlock {
		/* load shm */
		MemoryBlock();
		MemoryBlock(int key, int64_t size);

		int Attach();
		int Detach();

		~MemoryBlock();

		char * ptr;
		int key;
		int ref; /* reference count */
		int64_t size;
	};

	MemoryMgr();
	~MemoryMgr();

	/* @brief: Connect to Zookeeper, get metadata and initialize the SharedMemory
	 * @return: 0 if ok, -1 otherwise */
	int Init(const std::string &zklist, const std::string &host, int port);

	/* @brief: release all the shared memory */
	void Destroy(void);

	const std::string& GetZKList(void) const { return zklist_; }
	void SetZKList(const std::string &zklist) { zklist_ = zklist; }

	const std::string& GetHost(void) const { return host_ ; }
	void SetHost(const std::string &host) { host_ = host; }

	int GetPort(void) const { return port_; }
	void SetPort(int port) { port_ = port; }

	/* @brief: get the ptr for the given data id and version */
	const char * GetData(int data_id, int version, int64_t offset, /*out*/ int64_t &size);

	void ReleaseData(int data_id, int version);

	void UpdateData(std::map<int, std::map<int, MemoryBlock> > &);

	void PurgeData(void); /* purge the old data */

	/* for beginning's initialization */
	pthread_mutex_t lock_;
	pthread_cond_t cond_;
	pthread_mutex_t map_lock_;

	typedef std::map<int, std::map<int, MemoryBlock> > DataMap;
	typedef std::map<int, MemoryBlock> VersionMap;

	/* map<int data_id, map<int version, Memory> > */
	DataMap mmap_;

private:
	/* zk4cpp and zkhost */
	std::string zklist_;
	ZKClient zkcli_;

	/* ip and port */
	std::string host_;
	int port_;
};

}}}
#endif /* SRC_MEM_SYNC_SERVER_ZK_CLIENT_H_ */
