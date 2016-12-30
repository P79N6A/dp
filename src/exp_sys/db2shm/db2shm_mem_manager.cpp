#include "db2shm_mem_manager.h"

DEFINE_int32(exp_shm_key_1, 13579, "共享内存key之一，用于更新时切换");
DEFINE_int32(exp_shm_key_2, 24680, "共享内存key之一，用于更新时切换");
DEFINE_int32(exp_shm_size, 1024 * 1024 * 20, "共享内存大小");
DEFINE_int32(memsync_data_id, 1234, "内存同步的配置ID");
DEFINE_int32(ds_port, 1234, "内存同步的dataserver port");
DEFINE_string(zk_list, "127.0.0.1:2181", "zk_list");

namespace poseidon {
namespace exp_sys {

bool MemManager::init()
{
    if (_init)
        return true;
    void * shm_ptr_1 = util::shm::ShmAttach(FLAGS_exp_shm_key_1,
        FLAGS_exp_shm_size);
    if (shm_ptr_1 == NULL) {
        shm_ptr_1 = util::shm::ShmCreate(FLAGS_exp_shm_key_1,
            FLAGS_exp_shm_size);
        if (shm_ptr_1 == NULL) {
            LOG_ERROR("shm_ptr_1 init error,size : %d", FLAGS_exp_shm_size);
            return false;
        }
        memset(shm_ptr_1, 0, FLAGS_exp_shm_size);
    }
    void * shm_ptr_2 = util::shm::ShmAttach(FLAGS_exp_shm_key_2,
        FLAGS_exp_shm_size);
    if (shm_ptr_2 == NULL) {
        shm_ptr_2 = util::shm::ShmCreate(FLAGS_exp_shm_key_2,
            FLAGS_exp_shm_size);
        if (shm_ptr_2 == NULL) {
            LOG_ERROR("shm_ptr_2 init error,size : %d", FLAGS_exp_shm_size);
            return false;
        }
        memset(shm_ptr_2, 0, FLAGS_exp_shm_size);
    }
    LOG_INFO("shm init succ,size : %d", FLAGS_exp_shm_size);

    MemHead * shm_head_1 = (MemHead *) shm_ptr_1;
    MemHead * shm_head_2 = (MemHead *) shm_ptr_2;
    if (shm_head_1->version >= shm_head_2->version) {
        _version = shm_head_1->version + 1;
        _read_ptr = shm_ptr_1;
        _read_shm_key = FLAGS_exp_shm_key_1;
        _update_ptr = shm_ptr_2;
        _update_shm_key = FLAGS_exp_shm_key_2;
    } else {
        _version = shm_head_2->version + 1;
        _read_ptr = shm_ptr_2;
        _read_shm_key = FLAGS_exp_shm_key_2;
        _update_ptr = shm_ptr_1;
        _update_shm_key = FLAGS_exp_shm_key_1;
    }

    _init = true;
    return true;
}

bool MemManager::load(const vector<MemExpInfo>& exp_infos,
    const vector<MemExpParaInfo>& exp_para_infos)
{
    if (!_init)
        return false;
    uint32_t head_len = sizeof(MemHead);
    uint32_t exp_infos_len = sizeof(MemExpInfo) * exp_infos.size();
    uint32_t para_infos_len = sizeof(MemExpParaInfo) * exp_para_infos.size();
    _buffer_len = head_len + exp_infos_len + para_infos_len;
    while (1) {
        _buffer = new char[_buffer_len];
        if (_buffer == NULL)
            break;
        LOG_INFO("new buffer succ,size : %d", _buffer_len);
        MemHead * mem_head = (MemHead *) _buffer;
        mem_head->mark = EXP_SHM_MARK;
        mem_head->version = _version;
        mem_head->last_update_shm_time = time((time_t) NULL);
        mem_head->mem_exp_info_num = exp_infos.size();
        mem_head->mem_exp_para_info_num = exp_para_infos.size();

        for (int i = 0; i < exp_infos.size(); i++) {
            MemExpInfo * exp_info = ((MemExpInfo *) (_buffer + head_len)) + i;
            exp_info->mark = EXP_SHM_MARK;
            exp_info->exp_info_no = i;
            exp_info->module_id = exp_infos[i].module_id;
            exp_info->source_id = exp_infos[i].source_id;
            exp_info->view_type = exp_infos[i].view_type;
            exp_info->exp_id = exp_infos[i].exp_id;
            exp_info->exp_valid_from = exp_infos[i].exp_valid_from;
            exp_info->exp_valid_to = exp_infos[i].exp_valid_to;
            exp_info->exp_quota_from = exp_infos[i].exp_quota_from;
            exp_info->exp_quota_to = exp_infos[i].exp_quota_to;
            LOG_DEBUG(
                "[exp_info writen to mem]no=%d,module_id=%d,source_id=%d,view_type=%d,exp_id=%d,exp_quota_from=%d,exp_quota_to=%d",
                exp_info->exp_info_no, exp_info->module_id, exp_info->source_id,
                exp_info->view_type, exp_info->exp_id, exp_info->exp_quota_from,
                exp_info->exp_quota_to);
        }

        for (int i = 0; i < exp_para_infos.size(); i++) {
            MemExpParaInfo * para_info = ((MemExpParaInfo *) (_buffer + head_len
                + exp_infos_len)) + i;
            para_info->mark = EXP_SHM_MARK;
            para_info->exp_para_info_no = i;
            para_info->exp_id = exp_para_infos[i].exp_id;
            para_info->para = exp_para_infos[i].para;
            LOG_DEBUG(
                "[para_info writen to mem]no=%d,exp_id=%d,exp_para_key=%d",
                para_info->exp_para_info_no, para_info->exp_id,
                para_info->para.param_id);
        }
        if (!util::Func::md5sum(_buffer + head_len,
            exp_infos_len + para_infos_len, _md5)) {
            _loaded = false;
            break;
        }
        strncpy(mem_head->md5, _md5.c_str(), sizeof(mem_head->md5) - 1);
        LOG_INFO("load succ,md5 : %s", mem_head->md5);
        _loaded = true;
        break;
    }
    return _loaded;
}

bool MemManager::update_shm()
{
    MemHead * shm_head = (MemHead *) _read_ptr;
    LOG_INFO("last shm md5 : %s", shm_head->md5);
    if (strcmp(shm_head->md5, _md5.c_str()) == 0) {
        LOG_INFO("data is not changed");
        return true;
    } else {
        memcpy(_update_ptr, _buffer, _buffer_len);
        MemHead * shm_head = (MemHead *) _update_ptr;
        LOG_INFO("update_shm  shm,version : %d,md5 : %s", shm_head->version,
            shm_head->md5);
        string local_ip;
        if (poseidon::util::Func::get_local_ip(local_ip) != 0) {
            LOG_ERROR("can not get local ip");
            return false;
        }
        int ret = mem_sync::RegManager::get_mutable_instance().init(
            FLAGS_zk_list, local_ip, FLAGS_ds_port);
        if (ret < 0) {
            LOG_ERROR("connect to zk error,ret=%d", ret);
            return false;
        }
        int version = mem_sync::RegManager::get_mutable_instance().reg_data(
            FLAGS_memsync_data_id, _update_shm_key, FLAGS_exp_shm_size);
        if (version < 0) {
            LOG_ERROR("signal to zk false,ret=%d", version);
            return false;
        } else {
            LOG_INFO("signal to zk succ,version=%d shmkey=%d shmsize=%d",
                version, _update_shm_key, FLAGS_exp_shm_size);
        }
        return true;
    }
}

}
}
