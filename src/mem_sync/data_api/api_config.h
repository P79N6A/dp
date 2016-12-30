/**
 **/

#ifndef  _MEM_SYNC_DATA_API_API_CONFIG_H_ 
#define  _MEM_SYNC_DATA_API_API_CONFIG_H_

#include <iostream>
#include <string>
#include <list>


namespace poseidon
{
namespace mem_sync
{

class ApiConfig
{
public:

    enum
    {
        DEF_MAN_SHM_KEY=0x996,
    };
    
    ApiConfig()
    {
        man_shm_key_=DEF_MAN_SHM_KEY;
    }

    ApiConfig & set_zk_hostlist(const std::string & zk_hostlist)
    {
        zk_hostlist_=zk_hostlist;
        return *this;
    }

    ApiConfig & set_man_shm_key(int key)
    {
        man_shm_key_=key;
        return *this;
    }

    ApiConfig & add_data(int dataid )
    {
        data_list_.push_back(dataid);
        return *this;
    }

    int man_shm_key()
    {
        return man_shm_key_;
    }

    const char * zk_hostlist()
    {
        return zk_hostlist_.c_str();
    }

    std::list<int> & data_list()
    {
        return data_list_;
    }

private:
    std::string zk_hostlist_;
    int man_shm_key_;
    std::list<int> data_list_;

};
}
}

#endif   // ----- #ifndef _MEM_SYNC_DATA_API_API_CONFIG_H_  ----- 


