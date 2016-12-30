#include "db2shm_inc.h"
#include "db2shm_mysql_process.h"
#include "db2shm_mem_manager.h"

DEFINE_string(conf, "../etc/conf.cfg", "配置文件路径");
DEFINE_string(log_conf, "../etc/log4cpp.conf", "日志配置文件路径");
DEFINE_string(log_category, "db2shm", "日志类目设置,请不要更改");
DEFINE_bool(show_cur_shm_key, false, "获得当前共享内存key");

using namespace poseidon::exp_sys;

int main(int argc, char** argv) {
    google::SetVersionString("1.0.0");
    ::google::ParseCommandLineFlags(&argc, &argv, true);
    ::google::ReadFromFlagsFile(FLAGS_conf, argv[0], true);

    if (!LOG_INIT(FLAGS_log_conf, FLAGS_log_category)) {
        fprintf(stderr, "LOG_INIT error[%s, %s]\n", FLAGS_log_conf.c_str(),
            FLAGS_log_category.c_str());
        return -1;
    }
    LOG_INFO("LOG_INIT SUCCESS!");

    MysqlProcess mysql_process;
    mysql_process.run();
    MemManager::get_mutable_instance().init();
    if (FLAGS_show_cur_shm_key) {
        cout << "shm key is "
            << MemManager::get_mutable_instance().get_cur_shm_key() << endl;
    } else {
        MemManager::get_mutable_instance().load(mysql_process.get_exp_infos(),
            mysql_process.get_para_infos());
        MemManager::get_mutable_instance().update_shm();
    }
    return 0;
}
