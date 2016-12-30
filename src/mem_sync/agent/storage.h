/*
 * storage.h
 * Created on: 2016-10-28
 * Description: use json file to record which dataid we are interested in.
 */

#ifndef SRC_MEM_SYNC_AGENT_STORAGE_H_
#define SRC_MEM_SYNC_AGENT_STORAGE_H_

#include <list>
#include <set>
#include <fstream>
#include <string>
#include <boost/serialization/singleton.hpp>
#include "json/json.h"

namespace poseidon {
namespace mem_sync {
namespace agent {

class Storage :
	public boost::serialization::singleton<Storage> {
public:
	Storage();
	~Storage();

	int Init(const std::string &path);
	int DumpData();
	int LoadData();

	const std::map<int, int> & GetDataIDs();
	int AddDataID(int data_id, int version);

private:
	int version_;
	std::string path_;
	std::map<int, int> data_ids_; /* data_id, version */
};

}}}

#endif /* SRC_MEM_SYNC_AGENT_STORAGE_H_ */
