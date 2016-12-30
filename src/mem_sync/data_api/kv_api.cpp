#include "kv_api.h"
#include "log.h"
#include "data_api.h"
#include "mem_kv/mem_kv.h"

#define HASH_CAP (1 << 20)

namespace poseidon
{

namespace mem_sync
{

KVApi::KVApi()
{
    _cap = HASH_CAP;
    _da = NULL;
    _kvtb = NULL;
}

KVApi::~KVApi()
{
    if (_da != NULL) delete _da;
    if (_kvtb != NULL) delete _kvtb;
}

bool KVApi::init()
{
    bool r = true;

    if (_kvtb == NULL) _kvtb = new MemKV(_cap);

    if (_da == NULL) {
        _da = new DataApi();
        r = _da->init();
    }

    return r;
}

int KVApi::get(int dataid, const std::string &key, std::string &val, int *version)
{
    size_t val_size;
    char *val_data;

    int r = get(dataid, key.size(), key.data(), val_size, val_data, version);
    if (r == 0) {
        val.assign(val_data, val_size);
    }

    return r;
}

int KVApi::get(int dataid, size_t key_size, const char *key, size_t &val_size, 
                char * &val, int *version)
{
    int r;
    uint64_t shm_size;
    const void *shm_ptr;

    if (_da == NULL || _kvtb == NULL || key == NULL) {
        LOG_ERROR("invalid arguments");
        return -1;
    }

    do {
        r = _da->get_addr(dataid, shm_ptr, shm_size, version);
        if (r != 0) {
            LOG_ERROR("get shm fail, dataid=%d", dataid);
            r = -1;
            break;
        }

        r = _kvtb->get(key_size, key, val_size, val, (char *)shm_ptr, (size_t)shm_size);
        if (r == 1) {
            //LOG_DEBUG("not found key=%.*s", key_size, key);
        } else if (r < 0) {
            LOG_ERROR("get val fail, r=%d, key=%.*s", r, key_size, key);
        }

    } while(0);

    return r;
}

int KVApi::get_version(int dataid)
{
    int r, version = 0;
    
    if (_da == NULL) {
        LOG_ERROR("invalid arguments");
        return -1;
    }

    r = _da->get_version(dataid, version);
    if (r != 0) return -1;

    return version;
}

KVIter *KVApi::get_iter(int dataid)
{
    int r, version;
    uint64_t shm_size;
    const void *shm_ptr;

    if (_da == NULL || _kvtb == NULL) {
        LOG_ERROR("invalid arguments");
        return NULL;
    }

    r = _da->get_addr(dataid, shm_ptr, shm_size, &version);
    if (r != 0) {
        LOG_ERROR("get shm fail, dataid=%d", dataid);
        return NULL;
    }

    LOG_DEBUG("go iteration, dataid=%d, shm_version=%d", dataid, version);
    KVIter *iter = new KVIter();
    r = iter->init(_kvtb, (void *)shm_ptr, shm_size);
    if (r != 0) {
        delete iter;
        iter = NULL;
    }

    return iter;
}

KVIter::KVIter()
{
    _miter = NULL;
}

KVIter::~KVIter()
{
    if (_miter != NULL) delete _miter;
}

int KVIter::init(MemKV *kvtb, void *shm_ptr, size_t shm_size)
{
    _miter = kvtb->get_iter((char *)shm_ptr, shm_size);
    return (_miter != NULL ? 0 : -1);
}

int KVIter::next(size_t &key_size, char * &key, size_t &val_size, char * &val)
{
    return _miter->next(key_size, key, val_size, val);
}

}
}
