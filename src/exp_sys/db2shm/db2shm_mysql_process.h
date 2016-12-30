#pragma once

#include "db2shm_inc.h"
#include "../api/exp_comm.h"

namespace poseidon {
namespace exp_sys {

class MysqlProcess {
public:
    MysqlProcess()
    {
        _result = NULL;
        _run = false;
    }
    ~MysqlProcess()
    {
        if (_result != NULL)
            mysql_free_result(_result);
        mysql_close(&_mysql);
    }
    void run();
    const vector<MemExpParaInfo> & get_para_infos()
    {
        return _para_infos;
    }
    const vector<MemExpInfo> & get_exp_infos()
    {
        return _exp_infos;
    }

protected:
    MYSQL _mysql;
    MYSQL_RES *_result;
    bool _run;
    vector<MemExpParaInfo> _para_infos;
    vector<MemExpInfo> _exp_infos;
};

}
}
