/**
 **/

#include "mem_manager.h"

#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <stdio.h>

#include "util/shm.h"

namespace poseidon {
namespace mem_sync {

/* Init MemoryManager's Managements structure
 * key should always be 0x236
 */
int MemManager::init(int key)
{
    int rt = 0;
    do {
        if (init_) {
            return 0;
        }

        /* create a shared memory for management. */
        man_shm_addr_ = (LocalShmManage *) util::shm::ShmAttach(key,
            sizeof(LocalShmManage), 0600 | IPC_CREAT);
        if (man_shm_addr_ == NULL) {
            rt = -1;
            break;
        }
        man_shm_key_ = key;
        init_ = true;
    } while (0);
    return rt;
}

/* Generate a new key */
int MemManager::get_new_shm_key()
{
    int old_shm_key_idx_ = next_shm_key_idx_;
    do {
        int shm_key = SHM_KEY_START + next_shm_key_idx_;

        //测试共享内存是否存在
        int shmid = shmget(shm_key, 1, 0400);
        if (shmid < 0 && errno == ENOENT) {
            next_shm_key_idx_ = (next_shm_key_idx_ + 1) % SHM_MAX_NUM;
            return shm_key;
        }
        next_shm_key_idx_ = (next_shm_key_idx_ + 1) % SHM_MAX_NUM;
    } while (old_shm_key_idx_ != next_shm_key_idx_);
    return -1; //找不到合适的key
}

/**
 * this function can *NOT* be called spontaneously.
 */
int MemManager::update_data(int dataid, int version, uint64_t size,
    void * & addr)
{
    int rt = 0;
    do {
        if (dataid < 0 || dataid >= MAX_DATAID) {
            rt = -1;
            break;
        }

        /* this function has been called, can not alloc twice. */
        if (map_update_addr_.count(dataid) > 0) {
            rt = -2;
            break;
        }

        if (!init_) {
            rt = init(SHM_MAN_KEY);
            if (rt != 0) {
                break;
            }
        }
        LocalDataInfo & datainfo = man_shm_addr_->datainfo[dataid];
        if (datainfo.used_flag != MS_USED_FLAG) { //数据块没有使用到
            datainfo.status = STAT_INVALID;
            datainfo.used_flag = MS_USED_FLAG;
            datainfo.index = -1;
        }
        datainfo.sync_start_time = time(NULL);
        /* write to the shm new_index points to */
        int new_index = (datainfo.index + 1) % 2;
        datainfo.shminfo[new_index].version = version;
        datainfo.shminfo[new_index].shm_size = size;

        /* generate a new key for the new shared memory */
        int new_key = get_new_shm_key();
        if (new_key < 0) {
            rt = -3;
            break;
        }
        datainfo.shminfo[new_index].shm_key = new_key;

        int shmid = shmget(datainfo.shminfo[new_index].shm_key, size,
            0600 | IPC_CREAT);
        if (shmid < 0) {
            rt = -4;
            break;
        }

        datainfo.shminfo[new_index].shmid = shmid;
        addr = shmat(shmid, NULL, 0);
        if (addr == (void *) -1) {
            rt = -5;
            break;
        }

        /* mark the dataid as updating */
        map_update_addr_[dataid] = addr;

    } while (0);
    return rt;
}

/* Finish updating dataid
 *
 */
int MemManager::update_done(int dataid)
{
    int rt = 0;
    do {
        if (dataid < 0 || dataid >= MAX_DATAID) {
            rt = -1;
            break;
        }
        if (map_update_addr_.count(dataid) == 0) {
            rt = -2;
            break;
        }

        if (!init_) {
            rt = init(SHM_MAN_KEY);
            if (rt != 0) {
                break;
            }
        }

        util::shm::ShmDetach(map_update_addr_[dataid]);
        map_update_addr_.erase(dataid);

        //更新到新内存块
        LocalDataInfo & datainfo = man_shm_addr_->datainfo[dataid];
        if (datainfo.sync_start_time == 0) {
            rt = -3;
            break;
        }
        datainfo.sync_start_time = 0;
        int old_index = datainfo.index;
        int new_index = (datainfo.index + 1) % 2;
        datainfo.current_version = datainfo.shminfo[new_index].version;
        datainfo.index = new_index;
        datainfo.status = STAT_VALID;

        //旧的内存块做释放
        if (datainfo.shminfo[old_index].shm_key >= SHM_KEY_START
            && datainfo.shminfo[old_index].shm_key
                <= SHM_KEY_START + SHM_MAX_NUM) {
            util::shm::ShmDelete(datainfo.shminfo[old_index].shm_key,
                datainfo.shminfo[old_index].shm_size);
        }
    } while (0);
    return rt;
}

int MemManager::get_addr(int dataid, const void * & addr, uint64_t & size,
    int * version)
{
    int rt = 0;
    do {
        if (dataid < 0 || dataid >= MAX_DATAID) {
            rt = -1;
            break;
        }
        if (!init_) {
            rt = init(SHM_MAN_KEY);
            if (rt != 0) {
                break;
            }
        }
        LocalDataInfo & datainfo = man_shm_addr_->datainfo[dataid];
        if (datainfo.used_flag != MS_USED_FLAG
            || datainfo.status == STAT_INVALID) {
            //该dataid无数据
            rt = -1;
            break;
        }

        //api第一次访问
        if (mapAddr.count(dataid) == 0) {
            AddrInfo & addrinfo = mapAddr[dataid];
            rt = update_addrinfo(datainfo, addrinfo);
            if (rt != 0) {
                rt = -2;
                break;
            }
            addr = addrinfo.addr;
            size = addrinfo.size;
            if (version != NULL) {
                *version = addrinfo.version;
            }
            break;
        } else {
            time_t now = time(NULL);
            AddrInfo & addrinfo = mapAddr[dataid];
            if (now - addrinfo.last_check_time >= 1) {
                if (addrinfo.version != datainfo.current_version) {
                    rt = update_addrinfo(datainfo, addrinfo);
                    if (rt != 0) {
                        rt = -3;
                        break;
                    }
                }
                addrinfo.last_check_time = now;
            }
            addr = addrinfo.addr;
            size = addrinfo.size;
            if (version != NULL) {
                *version = addrinfo.version;
            }

        }
    } while (0);
    return rt;
}

int MemManager::update_addrinfo(const LocalDataInfo & datainfo,
    AddrInfo & addrinfo)
{
    int rt = 0;
    do {
        if (datainfo.current_version != addrinfo.version) {
            int index = datainfo.index;
            const ShmInfo & shminfo = datainfo.shminfo[index];
//            int shm_key=shminfo.shm_key;
            uint64_t shm_size = shminfo.shm_size;
            void * addr = shmat(shminfo.shmid, NULL, SHM_RDONLY);
            if (addr == (void *) -1) {
                rt = -1;
                break;
            }

            if (addrinfo.addr != NULL) {
                //释放老的共享内存
                util::shm::ShmDetach(addrinfo.addr);
            }

            //填充新的共享内存
            addrinfo.addr = addr;
            addrinfo.size = shm_size;
            addrinfo.version = datainfo.current_version;
            addrinfo.last_check_time = time(NULL);
        }
    } while (0);
    return rt;
}

int MemManager::get_data_info(int dataid, uint32_t *key, uint32_t *size, int *version)
{
    if (!exist(dataid)) {
        /* dataid not found */
        return -1;
    }
    LocalDataInfo & datainfo = man_shm_addr_->datainfo[dataid];
    *version = datainfo.current_version;
    *key = datainfo.shminfo[datainfo.index].shm_key;
    *size = datainfo.shminfo[datainfo.index].shm_size;
    return 0;
}

void MemManager::show_data_info(int dataid)
{
    if (dataid < 0 || dataid >= MAX_DATAID) {
        printf("dataid must be [0, %d)\n", MAX_DATAID);
        return;
    }
    if (!exist(dataid)) {
        printf("dataid not exist\n");
        return;
    }

    if (!init_) {
        int rt = init(SHM_MAN_KEY);
        if (rt != 0) {
            return;
        }
    }

    LocalDataInfo & datainfo = man_shm_addr_->datainfo[dataid];
    std::cout << "datainfo.used_flag:" << datainfo.used_flag << std::endl;
    std::cout << "datainfo.status:" << datainfo.status << std::endl;
    std::cout << "datainfo.sync_start_time:" << datainfo.sync_start_time
        << std::endl;
    std::cout << "datainfo.current_version:" << datainfo.current_version
        << std::endl;
    std::cout << "datainfo.index:" << datainfo.index << std::endl;
    for (int i = 0; i < 2; i++) {
        ShmInfo & shminfo = datainfo.shminfo[i];
        std::cout << "[" << i << "]" << "shminfo.shm_key:" << shminfo.shm_key
            << std::endl;
        std::cout << "[" << i << "]" << "shminfo.shm_size:" << shminfo.shm_size
            << std::endl;
        std::cout << "[" << i << "]" << "shminfo.version:" << shminfo.version
            << std::endl;
#if 0
        std::cout<<"["<<i<<"]"<<"shminfo.checksum:"<<shminfo.checksum<<std::endl;
        std::cout<<"["<<i<<"]"<<"shminfo.server_ip:"<<shminfo.server_ip<<std::endl;
        std::cout<<"["<<i<<"]"<<"shminfo.server_port:"<<shminfo.server_port<<std::endl;
#endif
        std::cout << "[" << i << "]" << "shminfo.shmid:" << shminfo.shmid
            << std::endl;
    }

    return;
}

bool MemManager::exist(int dataid)
{
    if (dataid < 0 || dataid >= MAX_DATAID) {
        return false;
    }

    if (!init_) {
        int rt = init(SHM_MAN_KEY);
        if (rt != 0) {
            return false;
        }
    }
    LocalDataInfo & datainfo = man_shm_addr_->datainfo[dataid];
    if (datainfo.used_flag == MS_USED_FLAG && datainfo.status == STAT_VALID) {
        return true;
    } else {
        return false;
    }
}

int MemManager::get_version(int dataid, int & version)
{
    int rt = 0;
    do {
        if (dataid < 0 || dataid >= MAX_DATAID) {
            rt = -1;
            break;
        }
        if (!init_) {
            rt = init(SHM_MAN_KEY);
            if (rt != 0) {
                break;
            }
        }

        if (!exist(dataid)) {
            version = 0;
            break;
        }
        LocalDataInfo & datainfo = man_shm_addr_->datainfo[dataid];
        version = datainfo.current_version;

    } while (0);
    return rt;
}

}
}

