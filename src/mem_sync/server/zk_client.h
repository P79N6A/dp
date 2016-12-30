/*
 * File Name: zkclient.h
 * Created on: 2016-10-21
 */

#ifndef ZKCLIENT_H_
#define ZKCLIENT_H_

#include <set>
#include "zk4cpp.h"

namespace poseidon {
namespace mem_sync {
namespace server {

class ZKClient : public Zk4Cpp {
public:
	ZKClient();
	void Init();

	/* when session expired reconnect zk */
	virtual void onConnect(void);
	virtual void onExpired(void);
	virtual void onNodeChanged(int state, const char * path);
	virtual void onNodeChildChanged(int state, const char * path);
private:
	void UpdateData(void);
	void UpdateData(int dataid);
	void UpdateData(int dataid, int version);

private:
	bool first_connect_;
	std::string zklist_;
	std::string host_;
	int port_;
	std::set<int> dataids_;
};

}}}


#endif /* SRC_MEM_SYNC_SERVER_ZKCLIENT_H_ */
