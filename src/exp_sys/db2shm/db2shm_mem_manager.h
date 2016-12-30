#pragma once

#include "db2shm_inc.h"

namespace poseidon {
namespace exp_sys {

class MemManager: public boost::serialization::singleton<MemManager> {
public:
    MemManager()
    {
        _buffer = NULL;
        _buffer_len = 0;
        _loaded = false;
        _version = 0;
        _init = false;
        _read_ptr = NULL;
        _read_shm_key = 0;
        _update_ptr = NULL;
        _update_shm_key = 0;
    }
    bool init();
    bool load(const vector<MemExpInfo>& exp_infos,
        const vector<MemExpParaInfo>& exp_para_infos);
    bool update_shm();
    int get_cur_shm_key()
    {
        return _read_shm_key;
    }
protected:
    char * _buffer;
    uint32_t _buffer_len;
    bool _loaded;
    string _md5;
    uint32_t _version;
    bool _init;
    void * _read_ptr;
    int _read_shm_key;
    void * _update_ptr;
    int _update_shm_key;

protected:

};

}
}
