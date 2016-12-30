/**
 **/

#ifndef  _EXP_SYS_API_EXP_API_H_ 
#define  _EXP_SYS_API_EXP_API_H_

#include <iostream>
#include <stdint.h>
#include <vector>

#include <boost/serialization/singleton.hpp>
#include "../api/exp_comm.h"
#include "util/mutex.h"
#include "data_api/data_api.h"

namespace poseidon {
namespace exp_sys {

class ExpApi: public boost::serialization::singleton<ExpApi> {
public:
    bool init();
    bool init(int mem_sync_shm_key);
    bool init(int shm_key, int shm_size);
    /**
     * @brief               获取实验参数
     * @param module_id     [IN], 模块ID
     * @param source        [IN], 取自rtb请求包
     * @param view_type     [IN], 取自rtb请求包
     * @param vr_exp_id     [IN], 返回的试验ID列表
     * @param vr_exp_param  [IN], 返回的实验参数
     * @return              success return 0, or return other
     **/
    int get_exp_param(int module_id, int source, int view_type,
            std::vector<int> & vr_exp_id, std::vector<ExpParam> & vr_exp_param);
protected:
    ExpApi() {
        init_ = false;
        shm_ptr_ = NULL;
    }
protected:
    bool init_;
    util::Mutex init_mutex_;
    mem_sync::DataApi data_api_;
    void * shm_ptr_;
};

}
}

#define EXP_API() poseidon::exp_sys::ExpApi::get_mutable_instance()

#endif   // ----- #ifndef _EXP_SYS_API_EXP_API_H_  ----- 

