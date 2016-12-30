#include "logcheck_inc.h"
#include "logcheck_server.h"

DEFINE_string(conf, "../etc/conf.cfg", "配置文件路径");
DEFINE_string(log_conf, "../etc/log4cpp.conf", "日志配置文件路径");
DEFINE_string(log_category, "log_check", "日志类目设置,请不要更改");
DEFINE_string(pid, "../run.pid", "pid文件路径");

DECLARE_int32(udp_serv_port);

int main(int argc, char** argv) {
    google::SetVersionString("1.0.16");
    ::google::ParseCommandLineFlags(&argc, &argv, true);
    ::google::ReadFromFlagsFile(FLAGS_conf, argv[0], true);

    poseidon::util::Func::DaemonInit();

    if (!poseidon::util::Func::single_instance(FLAGS_pid)) {
        fprintf(stderr, " %s already run...\n", argv[0]);
        exit(-1);
    }

    if (!LOG_INIT(FLAGS_log_conf, FLAGS_log_category)) {
        fprintf(stderr, "LOG_INIT error[%s, %s]\n", FLAGS_log_conf.c_str(),
                FLAGS_log_category.c_str());
        return -1;
    }
    LOG_INFO("LOG_INIT SUCCESS!");

    poseidon::log_check::LogCheckServer::get_mutable_instance().start_up();
    
    int last_print_time = 0;
    while (1) {
        sleep(1);
        int now_time = time((time_t) NULL);
        if (now_time - last_print_time > 30) {
            last_print_time = now_time;
            LOG_INFO("pvlog_checker running...");
        }
    }

    return 0;
}
