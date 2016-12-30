/**
 **/

#ifndef  _MEM_SYNC_DATA_API_REG_MANAGER_H_ 
#define  _MEM_SYNC_DATA_API_REG_MANAGER_H_

#include <string>
#include <boost/serialization/singleton.hpp>

namespace poseidon {

namespace mem_sync {
class RegZKData;

class RegManager: public boost::serialization::singleton<RegManager> {
public:
    int init(const std::string &zklist, int dataserver_port);

    int init(const std::string &zklist, const std::string &local_host, int dataserver_port);

    int reg_data(int data_id, int shm_key, int shm_size);

private:
    RegZKData *_regzk;
};

}
}

#endif
