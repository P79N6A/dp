/*
 * File Name: zkclient.h
 * Created on: 2016-10-21
 */

#ifndef ZKCLIENT_H_
#define ZKCLIENT_H_

#include "zk4cpp.h"

#include <set>
#include <boost/serialization/singleton.hpp>

#include "util/ipc_mq.h"

namespace poseidon {
namespace mem_sync {
namespace agent {

class ZKClient : public Zk4Cpp {
public:
	ZKClient();
	~ZKClient();

	int Init(const std::string &zklist);
	virtual void onConnect(void);
	virtual void onExpired(void);
	virtual void onNodeChanged(int state, const char * path);
	virtual void onNodeChildChanged(int state, char const * path);
	virtual void OnCreateNodeAsync(const std::string& addr, int rc);

	void UpdateData(void);
	void UpdateData(int data_id);
	void UpdateConfig(int data_id);

	void NotifyAgent(int data_id);
	void UpdateVersion(int data_id, int version);

	void ReportVersion(void);
	void ReportVersion(int data_id, int version);
private:
	std::string zklist_;
	std::string host_;
	int port_;
	std::set<int> dataids_;
	util::ipc::MQ mq_;
};

}}}


#endif /* ZKCLIENT_H_ */
