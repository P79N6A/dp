#ifndef  _MEM_SYNC_DATA_API_REG_ZK_DATA_H_
#define  _MEM_SYNC_DATA_API_REG_ZK_DATA_H_

#include "zk4cpp.h"
#include <boost/serialization/singleton.hpp>
#include <map>

namespace poseidon
{

namespace mem_sync
{

class RegZKData: public Zk4Cpp, public boost::serialization::singleton<RegZKData>
{
    public:
        int init(const std::string &zklist, int ds_port);

        int init(const std::string &zklist, const std::string &ds_host, int ds_port);

        void *get_new_shm(int data_id, int &shm_key, size_t shm_size);

        int reg_data(int data_id, int shm_key, void *shm_ptr, size_t shm_size);

        int reg_data(int data_id, int shm_key, size_t shm_size);

    private:
        struct ZKNodeData {
            int    data_id;
            int    shm_key;
            int    version;
            int    flags;
            size_t shm_size;
            void  *shm_ptr;
        };
 
        bool wait_connect_done();
        bool detect_connection();
        int reg_node(struct ZKNodeData &node_data, char *node_name, char *node_val);
        int reg_data_internal(struct ZKNodeData &node_data);
        int detach_old_shm(int data_id);
        int del_old_shm(int data_id);
        void *create_free_shm(int data_id, int &shm_key, size_t shm_size);

    private:
        int _ds_port;
        std::string _zklist;
        std::string _local_host;
        std::map<int, std::map<int, ZKNodeData> > _shm_map;
};

}
}

#endif
