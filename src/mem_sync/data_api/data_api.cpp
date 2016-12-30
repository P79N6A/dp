#include "data_api.h"
#include "util/shm.h"

namespace poseidon {

namespace mem_sync {

bool DataApi::init() {
    if (init_)
        return init_;

    init_mutex_.wlock();
    while (!init_) {
        mem_manager_ = &MemManager::get_mutable_instance();
        mem_manager_->init();
        ipc_mq_.init(IPC_MQ_KEY, 0660);
        init_ = true;
        break;
    }
    init_mutex_.unlock();
    return init_;
}

bool DataApi::init(int mem_sync_shm_key) {
    if (init_)
        return init_;

    init_mutex_.wlock();
    while (!init_) {
        mem_manager_ = &MemManager::get_mutable_instance();
        mem_manager_->init(mem_sync_shm_key);
        ipc_mq_.init(IPC_MQ_KEY, 0660);
        init_ = true;
        break;
    }
    init_mutex_.unlock();
    return init_;
}

/*
 * @brief: try to get the address associated with dataid,
 * if no data is return, DataApi will send a message
 * to the message queue to inform DataAgent to update
 * the data.
 */
int DataApi::get_addr(int dataid, const void * & addr, uint64_t & size,
        int * version) {

    if (mem_manager_->get_addr(dataid, addr, size, version) == 0) {
        return 0;
    } else {
        bool snd = false;
        time_t now_time = time((time_t *)NULL);
        boost::unordered_map<int, time_t>::iterator iter = last_snd_mq_.find(
                dataid);
        if (iter != last_snd_mq_.end()) {
            int last_time = iter->second;
            if (now_time - last_time > 10) {
                snd = true;
            }
        } else {
            snd = true;
        }
        if (snd) {
            ipc_mq_.push(MQ_TYPE_API, (void *) &dataid, sizeof(int));
            last_snd_mq_[dataid] = now_time;
        }
    }
    return -1;
}

int DataApi::get_version(int dataid, int &version) {
    return mem_manager_->get_version(dataid, version);
}

}
}
