#include "qp_inc.h"
#include "qp_server.h"
#include "qp_device_price_map.h"
#include "ip_search.h"
#include "qp_log_printer.h"

DEFINE_string(conf, "./etc/conf.cfg", "配置文件路径");
DEFINE_string(log_conf, "./etc/log4cpp.conf", "日志配置文件路径");
DEFINE_string(log_category, "qp", "日志类目设置,请不要更改");
DEFINE_bool(ha_on, false, "zookeeper注册开关");
DEFINE_string(zk_ip_list, "127.0.0.1:2181", "zookeeper服务列表");
DEFINE_string(pid, "../run.pid", "pid文件路径");
DEFINE_string(ip_tag_dict, "../data/ip.tag.dict", "ip地址字典");

DECLARE_int32(udp_serv_port);

int main(int argc, char** argv) {
    google::SetVersionString("2.1.36");
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

    poseidon::qp::LogPrinter::get_mutable_instance().startup();

    poseidon::qp::DevicePriceMap::get_mutable_instance().start_up();

    poseidon::qp::QpServer::get_mutable_instance().start_up();

    poseidon::qp::IpSearch::get_mutable_instance().init(FLAGS_ip_tag_dict);

    sleep(5);
    if (FLAGS_ha_on) {
        int nrt = HA_INIT(FLAGS_zk_ip_list);
        if (nrt != 0) {
            LOG_ERROR("HA_INIT error, rt[%d]", nrt);
            exit(-1);
        }
        string local_ip;
        if (poseidon::util::Func::get_local_ip(local_ip) != 0) {
            LOG_ERROR("can not get local ip");
            exit(-1);
        }
        nrt = HA_REG("qp", local_ip, FLAGS_udp_serv_port);
        if (nrt != 0) {
            LOG_ERROR("HA_REG error, rt[%d]", nrt);
            exit(-1);
        }
    }
    LOG_INFO("HA_INIT SUCCESS!");

    int last_print_time = 0;
    while (1) {
        sleep(1);
        int now_time = time((time_t) NULL);
        if (now_time - last_print_time > 30) {
            last_print_time = now_time;
            LOG_INFO("poseidon_qp running...");
        }
    }

    return 0;
}
