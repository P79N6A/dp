#include "db2shm_mysql_process.h"

DEFINE_string(mysql_host, "10.15.19.163", "mysql_host");
DEFINE_int32(mysql_port, 3306, "mysql_port");
DEFINE_string(mysql_user, "parus", "mysql_user");
DEFINE_string(mysql_pw, "parus", "mysql_pw");
DEFINE_string(mysql_table, "parus", "mysql_table");

namespace poseidon {
namespace exp_sys {

void MysqlProcess::run()
{
    if (_run)
        return;
    _run = true;
    mysql_init(&_mysql);
    mysql_options(&_mysql, MYSQL_SET_CHARSET_NAME, "utf8");
    mysql_options(&_mysql, MYSQL_INIT_COMMAND, "SET NAMES utf8");
    if (!mysql_real_connect(&_mysql, FLAGS_mysql_host.c_str(),
        FLAGS_mysql_user.c_str(), FLAGS_mysql_pw.c_str(),
        FLAGS_mysql_table.c_str(), FLAGS_mysql_port, NULL, 0)) {
        LOG_ERROR("mysql connect error,%s:%d", FLAGS_mysql_host.c_str(),
            FLAGS_mysql_port);
        return;
    }
    string sql = \
        "SELECT module_id, source, view_type, exp_id, unix_timestamp(exp_valid_from), "
        "unix_timestamp(exp_valid_to), exp_quota_from, exp_quota_to, k_id, v_type, v "
        "FROM exp_params_view WHERE exp_status = 6 AND NOW()>=exp_valid_from AND "
        "NOW() <= exp_valid_to order by module_id, source, view_type, exp_id;";

    LOG_DEBUG("sql : %s", sql.c_str());
    int res = mysql_query(&_mysql, sql.c_str());
    if (!res) {
        _result = mysql_store_result(&_mysql);
        if (_result) {
            MYSQL_ROW sql_row;

            while (sql_row = mysql_fetch_row(_result)) {
                MemExpInfo exp_info;
                exp_info.module_id = util::Func::to_int(sql_row[0]);
                exp_info.source_id = util::Func::to_int(sql_row[1]);
                exp_info.view_type = util::Func::to_int(sql_row[2]);
                exp_info.exp_id = util::Func::to_int(sql_row[3]);
                exp_info.exp_valid_from = util::Func::to_int(sql_row[4]);
                exp_info.exp_valid_to = util::Func::to_int(sql_row[5]);
                exp_info.exp_quota_from = util::Func::to_int(sql_row[6]);
                exp_info.exp_quota_to = util::Func::to_int(sql_row[7]);

                MemExpParaInfo para_info;
                para_info.exp_id = exp_info.exp_id;
                para_info.para.param_id = util::Func::to_int(sql_row[8]);
                string para_type = sql_row[9];
                if (para_type.compare("int") == 0) {
                    para_info.para.param_type = PT_INT;
                    para_info.para.param_vlaue.int_v = util::Func::to_int(
                        sql_row[10]);
                } else if (para_type.compare("float") == 0) {
                    para_info.para.param_type = PT_FLOAT;
                    para_info.para.param_vlaue.float_v = (float) atof(
                        sql_row[10]);
                } else {
                    continue;
                }
                _para_infos.push_back(para_info);
                if (_exp_infos.size() != 0) {
                    MemExpInfo tmp_info = _exp_infos[_exp_infos.size() - 1];
                    if (tmp_info.exp_id == exp_info.exp_id)
                        continue;
                }
                _exp_infos.push_back(exp_info);
            }
        } else {
            LOG_ERROR("mysql_store_result error,%s", mysql_error(&_mysql));
        }
    } else {
        LOG_ERROR("mysql_query error,%s", mysql_error(&_mysql));
    }
}

}
}
