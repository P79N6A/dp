/*
 * storage.h
 * Created on: 2016-10-28
 * Description: use json file to record which dataid we are interested in.
 */

#include "storage.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <set>
#include <map>
#include <utility>
#include "util/log.h"

namespace poseidon {
namespace mem_sync {
namespace agent {

Storage::Storage() : version_(0)
{
    /* DO NOTHING */
}

Storage::~Storage()
{

}

int Storage::Init(const std::string &path)
{
    path_ = path;

    int res = open(path.c_str(), O_CREAT, O_RDWR | 0600);
    if (res == -1) {
        LOG_ERROR("can not open agent config!");
        return -1;
    }
    close(res);
    return LoadData();
}

int Storage::DumpData(void)
{
    Json::Value root;
    root["version"] = Json::Value(version_);

    std::map<int, int>::iterator it;
    for (it = data_ids_.begin(); it != data_ids_.end(); it++) {
        Json::Value id_ver;
        id_ver["data_id"] = it->first;
        id_ver["version"] = it->second;
        root["data_ids"].append(id_ver);
    }

    Json::FastWriter writer;
    std::string json = writer.write(root);

    std::string tmp_path = path_ + ".tmp";
    std::fstream fs(tmp_path.c_str(),
        std::ios_base::in | std::ios_base::out | std::ios_base::trunc);

    fs << json;
    fs.close();

    rename(tmp_path.c_str(), path_.c_str());
    return 0;
}

/* Config file should be as follows:
 * {
 * 	  "version": 1,
 *    "data" : [
 *    	{
 *			"data_id": $data,
 *			"version": $versionID
 *		}
 *    ]
 * } */
int Storage::LoadData(void)
{
    std::fstream fs(path_.c_str());
    Json::Reader reader;
    Json::Value root;

    bool success = reader.parse(fs, root);
    if (!success) {
        LOG_WARN("Json Format Error!");
        return 0;
    }

    version_ = root["version"].asInt();
    Json::Value &data_ids = root["data_ids"];

    Json::ArrayIndex i;
    for (i = 0; i < data_ids.size(); i++) {
        Json::Value &id_ver = data_ids[i];
        int id, ver;
        id = id_ver["data_id"].asInt();
        ver = id_ver["version"].asInt();

        LOG_INFO("found data id: %d", id);
        data_ids_.insert(std::make_pair(id, ver));
    }

    fs.close();
    return 0;
}

const std::map<int, int>& Storage::GetDataIDs()
{
    return data_ids_;
}

int Storage::AddDataID(int data_id, int version)
{
    data_ids_[data_id] = version;
    return 0;
}

}  // namespace agent
}  // namespace mem_sync
}  // namespace poseidon




