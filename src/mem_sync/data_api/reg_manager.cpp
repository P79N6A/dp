#include "reg_manager.h"
#include "reg_zk_data.h"

#include <string.h>

namespace poseidon {

namespace mem_sync {

int RegManager::init(const std::string &zklist, int dataserver_port)
{
    _regzk = &RegZKData::get_mutable_instance();

    return _regzk->init(zklist, dataserver_port);
}

int RegManager::init(const std::string &zklist, const std::string &dataserver_ip, 
                    int dataserver_port)
{
    _regzk = &RegZKData::get_mutable_instance();

    return _regzk->init(zklist, dataserver_ip, dataserver_port);
}

int RegManager::reg_data(int data_id, int shm_key, int shm_size) 
{
    return _regzk->reg_data(data_id, shm_key, shm_size);
}


}
}
