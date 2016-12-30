/**
 **/
#include <stdio.h>
#include <iostream>
#include <wait.h>
#include <unistd.h>

#include "monitor_api.h"
#include "sn_attr.h"
#include "process_net.h"
#include "config.h"
#include "ha.h"
#include "util/log.h"
#include "util/func.h"
#include "comm_event.h"
#include "comm_event_interface.h"
#include "comm_event_factory.h"
#include "gflags/gflags.h"

using namespace poseidon::sn;
using namespace dc::common::comm_event;

DEFINE_string(conf, "../etc/conf.cfg", "配置文件路径");
DEFINE_string(pid, "../run.pid", "pid文件路径");

void usage(char * basename) {
    fprintf(stderr, "Usage:%s config_file", basename);
}

int create_workers() {
    int rt = 0;
    do {
        int i = 0;
        int worker_count = Config::get_mutable_instance().worker_count();
        std::map<int, int> proc_info;  //pid-->pidx;

        int pinx = 0; //进程INDEX
        while(1) {
            pid_t pid = fork();
            if(pid > 0) {
                proc_info[pid] = i;
                if( ++i < worker_count ) {
                    continue;
                } else {
                    //父进程:
                    //1.注册服务
                    if(Config::get_mutable_instance().ha_on()) {
                        int nrt = HA_INIT(Config::get_mutable_instance().zk_iplist());
                        if(nrt != 0) {
                            LOG_ERROR("HA_INIT error, rt[%d]", rt);
                            exit (-1);
                        }
                        nrt = HA_REG_Q("sn", Config::get_mutable_instance().local_ip(), Config::get_mutable_instance().server_port(), Config::get_mutable_instance().quota());
                        if(nrt != 0) {
                            LOG_ERROR("HA_REG error, rt[%d]", rt);
                            exit (-1);
                        }
                    }
                    //2.等待子进程异常退出，重启进程
                    while(1) {
                        int status;
                        pid_t pid_exit = wait(&status);
                        if(pid_exit < 0) {
                            LOG_ERROR("wait 异常退出");
                            continue;
                        }
                        MON_ADD(ATTR_SN_RESTART, 1); 
                        pinx = proc_info[pid_exit];
                        LOG_ERROR("子进程退出重启,pindex[%d]pid[%d]", pinx, pid_exit);
                        pid_t pid_new = fork();
                        if (pid_new > 0) {
                            //记录新的进程ID
                            proc_info[pid_new] = pinx;
                            continue;
                        } else if(pid_new == 0) {
                            break;
                        }
                    }
                    //退出重新创建的子进程
                    break;

                }
            } else if(pid == 0) {
                pinx = i;
                break;
            }
        }

        /*设置进程ID，表示这是第pidx个进程*/
        Config::get_mutable_instance().process_idx(pinx);
    } while(0);
    return rt;
}

/**
 * @brief  初始化环境
 **/
int init(int argc, char * argv []) {
    int rt = 0;
    do {
#if 0
        if(argc < 2) {
            usage(argv[0]);
            rt = -1;
            break;
        }
#endif
        google::SetVersionString("2.0.5");
        ::google::ParseCommandLineFlags(&argc, &argv, true);

        /*step 1: 读取配置文件*/
        if(Config::get_mutable_instance().parse(FLAGS_conf.c_str()) != 0) {
            rt = -2;
            break;
        }

        /*step 2: push process into daemon*/
        poseidon::util::Func::DaemonInit();

        if(!poseidon::util::Func::single_instance(FLAGS_pid) ) {
            fprintf(stderr, "log_server is running..." );
            rt = -3;
            break;
        }

        /*step 3: LOG_INIT*/
        if(!LOG_INIT(   Config::get_mutable_instance().log_conf(),
                        Config::get_mutable_instance().log_category()
                    )) {
            fprintf(stderr, "LOG_INIT error[%s, %s]\n", Config::get_mutable_instance().log_conf(), Config::get_mutable_instance().log_category());
            rt = -1;
            break;
        }

        LOG_INFO("LOG_INIT SUCCESS!");

        rt = CommFactoryInterface::instance().init();
        if(rt != 0) {
            LOG_ERROR("CommFactoryInterface::instance().init() error, return [%d]", rt);
            rt = -2;
            break;
        }
        int ServPort = Config::get_mutable_instance().server_port();
        /*step 4: 创建Net实例*/
        ProcessNet &proc = ProcessNet::get_mutable_instance();
        if (proc.init() != 0) {
            rt = -3;
            break;
        }

        proc.bindaddr(Config::get_mutable_instance().local_ip(), ServPort);

        /*step 5: 创建多个进程, 让所有进程都监听相同的Net端口*/
        rt = create_workers();
        if(rt != 0) {
            LOG_ERROR("create_workers error[%d]\n", rt);
            break;
        }

        /*here: father process exit, and run in child process*/
        if(CommFactoryInterface::instance().re_init() != 0) {
            LOG_ERROR("re_init error\n");
            rt = -1;
            break;
        }

        CommFactoryInterface::instance().add_comm(&proc);

    } while(0);
    return rt;
}

int run() {
    int rt = 0;
    do {
        CommFactoryInterface::instance().run();
    } while(0);
    return rt;
}

#ifndef FOR_UNIT_TEST
int main(int argc, char * argv []) {
    int rt = 0;
    do {
        rt = init(argc, argv);
        if(rt != 0) {
            fprintf(stderr, "%s init error, return[%d]\n", argv[0], rt);
            break;
        }
        rt = run();
        fprintf(stderr, "%s process will exit, rt[%d]\n", argv[0], rt);

    } while(0);
    return rt;
}
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "util/proto_helper.h"

namespace poseidon
{
namespace sn
{
struct SessData
{
    uint32_t user_city_id;
    uint64_t time_sn_req;
    uint64_t time_sn_query_start;
    uint64_t time_sn_query_end;
    uint64_t time_sn_rsp;
    SNRequest sn_req;
    SNResponse sn_rsp;
    scoring::ScoringRequest scor_req;
    scoring::ScoringResponse scor_rsp;
};
}
}

using namespace poseidon::sn;
using namespace poseidon::util;
void TestRequest(ProcessNet &proc, const char *data)
{
    SessData *sess = new SessData();
    SNRequest &sn_req = sess->sn_req;
    ParseProtoFromTextFormatFile(data, &sn_req);
    std::string str;
    sn_req.SerializeToString(&str);
    fprintf(stdout, "%s", sn_req.DebugString().c_str());
	
    proc.DoRequest(sess);
    delete sess;
}

int main(int argc, char *argv[])
{
    ProcessNet &proc = ProcessNet::get_mutable_instance();
    proc.init();

    TestRequest(proc, argv[1]);

    return 0;
}
#endif
