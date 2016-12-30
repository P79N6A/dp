/**
 **/
#ifndef  _MEM_SYNC_DATA_API_DATA_API_H_ 
#define  _MEM_SYNC_DATA_API_DATA_API_H_

#include "util/mutex.h"
#include <boost/unordered_map.hpp>
#include "../mem_manager/mem_manager.h"
#include "util/ipc_mq.h"

namespace poseidon {
namespace mem_sync {

class DataApi {
public:
    DataApi() : init_(false), mem_manager_(NULL) {}
    /**
     * @brief               初始化
     */
    bool init();
    bool init(int mem_sync_shm_key);

    bool is_inited() {
        return init_;
    }

    /**
     * @brief               根据一个data_id，获取对应的内存块
     * @param dataid        [IN],数据id，对应一个数据块
     * @param addr          [OUT], 返回内存快的地址
     * @param size          [OUT], 返回内存块的大小
     * @param version       [OUT], 如果不为NULL，返回version,使用者如果需要知道数据是否改变，这个用这个字段做判断
     * @return              0 if success, or other code
     **/
    int get_addr(int dataid, const void * & addr, uint64_t & size,
            int * version = NULL);

    /**
     * @brief               返回dataid共享内存钟的版本
     * @param dataid        [IN]
     * @param version       [OUT], 返回dataid的版本,如果dataid不存在，返回0
     * @return              success return 0, or return other
     **/
    int get_version(int dataid, int & version);

    /**
     * @brief               打印单个内存块的信息
     **/
    void show_data_info(int dataid) {}

private:
    boost::unordered_map<int, time_t> last_snd_mq_;
    bool init_;
    util::Mutex init_mutex_;
    MemManager *mem_manager_;
    util::ipc::MQ ipc_mq_;
};

}

}

#endif   // ----- #ifndef _MEM_SYNC_DATA_API_DATA_API_H_  ----- 

