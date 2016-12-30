#include "kv_reg_mgr.h"

#include <string.h>

#include "log.h"
#include "reg_zk_data.h"
#include "shm.h"
#include "mem_kv/mem_kv.h"

#define HASH_CAP (1 << 20)

namespace poseidon
{

namespace mem_sync
{

KVRegMgr::KVRegMgr()
{
    _cap = HASH_CAP;
    _regzk = NULL;
    _kvtb = NULL;
}

KVRegMgr::~KVRegMgr()
{
    if (_kvtb != NULL) delete _kvtb;
}

int KVRegMgr::init(const std::string &zklist, int dataserver_port)
{
    int r = 0;

    if (_regzk == NULL) {
        _regzk = &RegZKData::get_mutable_instance();
        r = _regzk->init(zklist, dataserver_port);
    }

    return r;
}

int KVRegMgr::init(const std::string &zklist, const std::string &dataserver_ip, 
                    int dataserver_port)
{
    int r = 0;

    if (_regzk == NULL) {
        _regzk = &RegZKData::get_mutable_instance();
        r = _regzk->init(zklist, dataserver_ip, dataserver_port);
    }

    return r;
}

int KVRegMgr::put(const std::string &key, const std::string &val)
{
    return put(key.size(), key.data(), val.size(), val.data());
}

int  KVRegMgr::put(size_t key_size, const char *key, size_t val_size, const char *val)
{
    if (key == NULL || val == NULL) {
        LOG_ERROR("invalid arguments");
        return -1;
    }

    if (_kvtb == NULL) {
        _kvtb = new MemKV(_cap);
    }
    
    return _kvtb->put(key_size, key, val_size, val);
}

void KVRegMgr::reset()
{
    if (_kvtb != NULL) {
        delete _kvtb;
        _kvtb = NULL;
    }
}

int KVRegMgr::reg_data(int data_id)
{
    int r = 0, shm_key;
    char *shm_ptr = NULL;
    
    if (_kvtb == NULL || _regzk == NULL) {
        LOG_ERROR("invalid arguments");
        return -1;
    }

    if (data_id <= 0) {
        LOG_ERROR("negtive data_id=%d", data_id);
        return -1;
    }

    do {
        size_t shm_size = _kvtb->calc_seri_size();
        shm_ptr = (char *)_regzk->get_new_shm(data_id, shm_key, shm_size);
        if (shm_ptr == NULL) {
            r = -3;
            break;
        }

        r = _kvtb->serialize((char *)shm_ptr, shm_size);
        if (r != 0) {
            r = -4;
            break;
        }

        r = _regzk->reg_data(data_id, shm_key, shm_ptr, shm_size);

    } while(0);

    delete _kvtb;
    _kvtb = NULL;

    if (r < 0) {
        LOG_ERROR("reg fail, rc=%d", r);
        if (shm_ptr != NULL) {
            util::shm::ShmDetach(shm_ptr);
            util::shm::ShmDelete(shm_key, 1);
        }
    }

    return r;
}

}
}
