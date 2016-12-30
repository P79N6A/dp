/**
 **/

#ifndef  _MEM_SYNC_DATA_AGENT_MEM_MANAGER_H_ 
#define  _MEM_SYNC_DATA_AGENT_MEM_MANAGER_H_

#include <iostream>
#include <map>
#include <boost/serialization/singleton.hpp>

#include "../common/comm_macro.h"

namespace poseidon {
namespace mem_sync {

class MemManager: public boost::serialization::singleton<MemManager> {

public:

    MemManager() : man_shm_key_(0), init_(false),
        man_shm_addr_(NULL), next_shm_key_idx_(0) {
    }

    /**
     * @brief               初始化
     * @param key           [IN],管理内存key
     * @return              success return 0, or return other
     **/
    int init(int key = SHM_MAN_KEY);

    /**
     * @brief               添加新数据,供写
     * @param dataid        [IN]
     * @param version       [IN],数据的版本
     * @param size          [IN],数据的大小
     * @param addr          [OUT],返回内存块,供写入
     * @return              success return 0, or return other
     **/
    int update_data(int dataid, int version, uint64_t size, void * & addr);

    /**
     * @brief               写完成调用
     * @param dataid        [IN]
     * @param key           [OUT] 获取映射的 shmkey值
     * @return              success return 0, or return other
     **/
    int update_done(int dataid);

    /**
     * @brief               返回dataid共享内存钟的版本
     * @param dataid        [IN]
     * @param version       [OUT], 返回dataid的版本,如果dataid不存在，返回0
     * @return              success return 0, or return other
     **/
    int get_version(int dataid, int & version);

    /**
     * @brief               获取数据块
     * @param dataid        [IN]
     * @param addr          [OUT], 返回数据块
     * @param size          [OUT], 数据块的大小
     * @param version       [OUT], 非NULL，返回版本号
     * @return              success return 0, or return other
     **/
    int get_addr(int dataid, const void * & addr, uint64_t & size,
            int * version = NULL);

    /**
     * @brief               显示data相关信息，供调试
     **/
    void show_data_info(int dataid);

    int get_data_info(int dataid, uint32_t *key, uint32_t *size, int *version);

    /**
     * @brief               判断一个dataid是否存在
     * @return              exist return true, or false
     **/
    bool exist(int dataid);

private:

    struct ManageHead {

    };

    struct ShmInfo {
        uint32_t shm_key;
        uint32_t shm_size;
        uint32_t version;
#if 0
        char checksum[CHECK_SUM_MAX];
        char server_ip[IP_LEN];
        uint32_t server_port;
#endif
        int shmid;
    };

    enum {
        STAT_INVALID = 0, STAT_VALID = 1,
    };

    struct LocalDataInfo {
        int used_flag;
        int status;         //状态,STAT_INVALID-不可用,STAT_VALID
        int sync_start_time;
        int current_version;
        int index;
        ShmInfo shminfo[2];
    };

    /**
     * @brief           管理共享内存块的布局
     **/
    struct LocalShmManage {
        ManageHead head;
        LocalDataInfo datainfo[MAX_DATAID + 1];
    };

    struct AddrInfo {
        void * addr;        //地址
        uint64_t size;          //大小
        int version;        //版本号
        int last_check_time;        //上次检查的时间
        AddrInfo() :
                addr(NULL), size(0), version(0), last_check_time(0) {
        }
    };

    /**
     * @brief               获取一个新的共享内存的key
     **/
    int get_new_shm_key();

    /**
     * @brief               更新地址信息
     * @param datainfo      [IN],输入datainfo
     * @param addrinfo      [OUT], 返回地址信息
     * @return              success return 0, or return other
     **/
    int update_addrinfo(const LocalDataInfo & datainfo, AddrInfo & addrinfo);

    int man_shm_key_;
    int init_;

    LocalShmManage * man_shm_addr_;
    int next_shm_key_idx_;
    std::map<int, AddrInfo> mapAddr;

    std::map<int, void *> map_update_addr_;  // mark if the dataid is updating
};

}

}


#define MM_INST() poseidon::mem_sync::MemManager::get_mutable_instance()

#endif   // ----- #ifndef _MEM_SYNC_DATA_AGENT_MEM_MANAGER_H_  ----- 

