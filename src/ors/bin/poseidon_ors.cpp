/**
 **/
#include <stdio.h>
#include <iostream>
#include <getopt.h>

#include "util/log.h"
#include "util/pvlog.h"
#include "util/func.h"
#include "src/ha/ha.h"
#include "src/ors/bin/config.h"
#include "src/ors/bin/process_net.h"

using namespace poseidon::ors;
using namespace dc::common::comm_event;


#define VERSION "poseidon_ors version 1.1.0"

void usage(const char * program) 
{
    fprintf(stderr, "%s [options]\n", program);
    fprintf(stderr, "    -h|--help show help\n");
    fprintf(stderr, "    -c|--conf specify the config file\n");
    fprintf(stderr, "    -v|--version show version\n");
}

int init(const char* conf_file)
{
    int rt=0;
    do{
        /*step 1: 读取配置文件*/
        if(Config::get_mutable_instance().parse(conf_file) != 0)
        {
            rt=-2;
            break;
        }

        /*step 2: push process into daemon*/
        poseidon::util::Func::DaemonInit();

        /*step 3: LOG_INIT*/
        if(!LOG_INIT(Config::get_mutable_instance().log_conf(),Config::get_mutable_instance().log_category()))
        {
            fprintf(stderr, "LOG_INIT error [%s, %s]\n", Config::get_mutable_instance().log_conf(), Config::get_mutable_instance().log_category());
            rt=-3;
            break;
        }

        LOG_INFO("LOG_INIT SUCCESS!");

        if (!poseidon::util::Func::single_instance("../run.pid")) {
            LOG_ERROR("Create PidFile failed!");
            rt = -10;
            break;
        }

        rt=CommFactoryInterface::instance().init();
        if(rt != 0)
        {
            LOG_ERROR("CommFactoryInterface::instance().init() error, return [%d]", rt);
            rt=-4;
            break;
        }

        /*step 4: 创建多个进程, 让所有进程都监听相同的Net端口*/
        int worker_count=Config::get_mutable_instance().worker_count() -1 ;
        int pinx=0;   //进程INDEX

        for (int i = 0; i < worker_count; i++) {
            pid_t pid=fork();
            if (pid == 0) {
                pinx = i + 1;
                break;
            } else if (pid > 0) {
                LOG_INFO("fork child pid=%d ok", pid);
            } else
            {
                LOG_ERROR("fork error");
                exit(-1);
            }
        }
        /*
        while(worker_count > 0)
        {
            pid_t pid=fork();
            if(pid >0)
            {
                if( ++i < worker_count )
                {
                    continue;
                }else
                {
                    //父进程退出
                    //exit(0);
                }
            }else if(pid == 0)
            {
                pinx=i;
                break;
            }
        }*/
        /*设置进程ID，表示这是第pidx个进程*/
        Config::get_mutable_instance().process_idx(pinx);

        /*here: father process exit, and run in child process*/
    	if(CommFactoryInterface::instance().re_init() != 0)
        {
        	LOG_ERROR("re_init error\n");
        	return -5;
        }

        int OffPort=Config::get_mutable_instance().off_port()*pinx;
        int ServPort=Config::get_mutable_instance().server_port()+OffPort;

        /*step 5: 创建Net实例*/
        ProcessNet::get_mutable_instance().bindaddr(Config::get_mutable_instance().local_ip(), ServPort);

        CommFactoryInterface::instance().add_comm(&(ProcessNet::get_mutable_instance()));

        if(ProcessNet::get_mutable_instance().Init(Config::get_mutable_instance().algo_conf_path()) != 0)
        {
            LOG_ERROR("ProcessNet Init Failed!");
            return -6;
        }

        if(Config::get_mutable_instance().ha_on())
        {
            int nrt=HA_INIT(Config::get_mutable_instance().zk_iplist());
            if (nrt != 0)
            {
                LOG_ERROR("HA_INIT Falied!ZkIplist=%s", Config::get_mutable_instance().zk_iplist());
                return -7;

            }
            LOG_INFO("HA_INTI OK!ZkIplist=%s", Config::get_mutable_instance().zk_iplist());
            if (HA_REG("ors", Config::get_mutable_instance().local_ip(), ServPort) != 0)
            {
                LOG_ERROR("HA_REG ors Failed!local_ip=%s, ServPort=%d", Config::get_mutable_instance().local_ip(), ServPort);
                return -8;
            }
            LOG_INFO("HA_REG ors OK!local_ip=%s, ServPort=%d", Config::get_mutable_instance().local_ip(), ServPort); 
        }
        char pvfilename[256];
        snprintf(pvfilename, 256, "ors-%d", pinx);
        PV_NAME(pvfilename);

        LOG_INFO("-----ors init Finish-----");
    }while(0);
    return rt;
}

int run()
{
    int rt=0;
    do{
        LOG_INFO("-----ors start to run-----");
        CommFactoryInterface::instance().run();
    }while(0);
    return rt;
}

int main(int argc, char * argv [])
{
    struct option long_options[] = {
        { "help", no_argument, NULL, 'h' },
        {"conf", required_argument, NULL, 'c' },
        { "version", no_argument, NULL, 'v' },
        { NULL, 0, NULL, 0 }  // sentinel
    };

    int c = 0;
    std::string conf_file = "";
    while ((c = getopt_long(argc, argv, "hvc:f", long_options, NULL)) != -1) {
        switch (c) {
        default:
        case 'h':
            usage(argv[0]);
            exit(0);
        case 'c':
            conf_file = optarg;
            break;
        case 'v':
            fprintf(stdout, "%s\n", VERSION);
            exit(0);
        }
    }

    if (conf_file.empty())
    {
        fprintf(stderr, "conf must be specified!\n");
        usage(argv[0]);
        exit(1);
    }


    int rt=0;
    do{
        rt=init(conf_file.c_str());
        if(rt != 0)
        {
            fprintf(stderr, "%s init error, return[%d]\n", argv[0], rt);
            break;
        }
        rt=run();
        fprintf(stderr, "%s process will exit, rt[%d]\n", argv[0], rt);

    }while(0);
    return rt;
}
