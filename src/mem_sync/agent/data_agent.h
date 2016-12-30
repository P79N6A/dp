/*
 * data_agent.h
 * Created on: 2016-10-24
 */

#ifndef DATA_AGENT_H_
#define DATA_AGENT_H_

#include <time.h>
#include <pthread.h>
#include <map>
#include <boost/serialization/singleton.hpp>
#include "event2/event.h"
#include "comm_event_tcp_listener.h"
#include "util/ipc_mq.h"
#include "zk_client.h"

using poseidon::util::ipc::MQ;

namespace poseidon {
namespace mem_sync {
namespace agent {

class DataAgent :
	public dc::common::comm_event::CommTcpListener,
	public boost::serialization::singleton<DataAgent> {
public:
	enum {
		UNUSED   = 0, 		/* the memory block non-exists */
		USED     = 1 << 0,
		PENDING  = 1 << 1,  /* the memory block is about to update */
		UPDATING = 1 << 2,  /* the memory block is updating */
		BAD      = 1 << 3,  /* the memory block is bad, we should not update again */
	};

	struct MemStat {
		MemStat() : stat(UNUSED), cur_ver(0), new_ver(0), host(""), port(0) {}
		int stat; /* one of the above enum value */
		int cur_ver; /* the cur version */
		int new_ver; /* new version */
		std::string host; /* which server we should connect to */
		int port;
		std::string md5; /* new version's MD5 */
	};

	DataAgent();
	~DataAgent();

	/* virtual function from CommTcpListener */
	virtual int init(int fd = -1, int status = STAT_INITED);
	virtual void on_timer(void);

	void InitDataIDs(const std::map<int, int> &ids);

	int UpdateData(int data_id, bool add);
	int UpdateMemStat(int data_id, int version, int stat);

	void DumpMemStat(void);

	/* cache */
	typedef std::map<int, MemStat> StatMap;
	StatMap map_;

	/* temporarily in used, will be removed in future. */
	pthread_mutex_t lock_;
private:
	struct event * timer_; /* used for fetch message from MQ */
	ZKClient zkcli_;
	MQ msgq_;
	int msgid_; /* system V msg queue id*/
};

}}}

#endif /* DATA_AGENT_H_ */
