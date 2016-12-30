/**
 **/

#include "exp_api.h"
#include "util/shm.h"
#include <boost/unordered_set.hpp>

namespace poseidon {
namespace exp_sys {

bool ExpApi::init() {
    if (init_)
        return init_;

    srand(time(0));
    init_mutex_.wlock();
    while (!init_) {
        init_ = data_api_.init();
        break;
    }
    init_mutex_.unlock();
    return init_;
}

/* memory manager's management structure key
 * should always be 0x236
 */
bool ExpApi::init(int mem_sync_shm_key) {
    if (init_)
        return init_;

    srand(time(0));
    init_mutex_.wlock();
    while (!init_) {
        init_ = data_api_.init(mem_sync_shm_key);
        break;
    }
    init_mutex_.unlock();
    return init_;
}

bool ExpApi::init(int shm_key, int shm_size) {
    if (init_)
        return init_;

    srand(time(0));
    init_mutex_.wlock();
    while (!init_) {
        shm_ptr_ = util::shm::ShmAttach(shm_key, shm_size);
        if (shm_ptr_ == NULL) {
            break;
        }
        init_ = true;
        break;
    }
    init_mutex_.unlock();
    return init_;
}

int ExpApi::get_exp_param(int module_id, int source, int view_type,
        std::vector<int> & vr_exp_id, std::vector<ExpParam> & vr_exp_param) {
    if (!init_)
        return -1;
    const void * mem_addr;
    uint64_t mem_size;
    if (shm_ptr_ == NULL) {
        if (data_api_.get_addr(EXP_SYS_DATA_ID /* data_id */, mem_addr, mem_size) != 0)
            return -2;
    } else {
        mem_addr = shm_ptr_;
    }
    MemHead * mem_head = (MemHead *) mem_addr;
    if (mem_head->mark != EXP_SHM_MARK) {
        return -3;
    }
    int exp_quota = rand() % 1000;
    uint32_t exp_info_num = mem_head->mem_exp_info_num;
    uint32_t exp_para_info_num = mem_head->mem_exp_para_info_num;
    uint32_t head_len = sizeof(MemHead);
    uint32_t exp_infos_len = sizeof(MemExpInfo) * exp_info_num;

    int now_time = time((time_t) NULL);
    boost::unordered_set<int> exp_id_set;
    for (int i = 0; i < exp_info_num; i++) {
        MemExpInfo * exp_info = ((MemExpInfo *) (mem_addr + head_len)) + i;
        if (exp_info->mark != EXP_SHM_MARK)
            break;
        if (exp_info->exp_info_no != i)
            break;
        bool insert = false;
        //view_type为-1时，匹配到所有
        if (exp_info->module_id == module_id && exp_info->source_id == source
                && exp_info->view_type == -1) {
            insert = true;
        }
        if (exp_info->module_id == module_id && exp_info->source_id == source
                && exp_info->view_type == view_type) {
            insert = true;
        }
        if (insert) {
            if (now_time >= exp_info->exp_valid_from
                    && now_time < exp_info->exp_valid_to) {
                if (exp_quota >= exp_info->exp_quota_from
                        && exp_quota <= exp_info->exp_quota_to) {
                    vr_exp_id.push_back(exp_info->exp_id);
                    exp_id_set.insert(exp_info->exp_id);
                }
            }
        }
    }

    for (int i = 0; i < exp_para_info_num; i++) {
        MemExpParaInfo * para_info = ((MemExpParaInfo *) (mem_addr + head_len
                + exp_infos_len)) + i;
        if (para_info->mark != EXP_SHM_MARK)
            break;
        if (para_info->exp_para_info_no != i)
            break;
        if (exp_id_set.count(para_info->exp_id) > 0) {
            vr_exp_param.push_back(para_info->para);
        }
    }
    return 0;
}

}
}

