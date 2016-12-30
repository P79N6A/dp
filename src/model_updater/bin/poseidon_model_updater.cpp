/**
 **/
#include <stdio.h>
#include <iostream>
#include <signal.h>
#include <sys/param.h>
#include <getopt.h>

#include "util/log.h"
#include "util/func.h"
#include "src/model_updater/bin/config.h"
#include "src/model_updater/model/data_watcher.h"
#include "src/mem_sync/data_api/kv_reg_mgr.h"

#define VERSION "dataagent version 1.0.4"

void usage(const char * program) 
{
    fprintf(stderr, "%s [options]\n", program);
    fprintf(stderr, "    -h|--help show help\n");
    fprintf(stderr, "    -c|--conf specify the config file\n");
    fprintf(stderr, "    -v|--version show version\n");
}

using namespace poseidon::model_updater;

DataWatcher g_watcher;

void DaemonInit()
{
  int fd;

   // shield some signals
   signal(SIGALRM, SIG_IGN);
   signal(SIGINT,  SIG_IGN);
   signal(SIGHUP,  SIG_IGN);
   signal(SIGQUIT, SIG_IGN);
   signal(SIGPIPE, SIG_IGN);
   signal(SIGTTOU, SIG_IGN);
   signal(SIGTTIN, SIG_IGN);
   signal(SIGCHLD, SIG_IGN);
   signal(SIGTERM, SIG_IGN);

   // fork child process
   if (fork()) exit(0);

   // creates  a new session
	if (setsid() == -1)  exit(1);

	// If you needn't STDIN,STDOUT,STDERR, close fd from 0;
	for(fd=3;fd<NOFILE;fd++) close(fd);
	//chdir("/");
	umask(0);
	return;
}

void SignalHandler(int signum)
{    
    LOG_INFO("caught signal [%d]", signum);
    g_watcher.Terminate();
}

/**
 * @brief  初始化环境
 **/
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
        DaemonInit();

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
            rt = -6;
            break;
        }

        // 注册信号处理    
        signal(SIGTERM, SignalHandler);    
        signal(SIGINT, SignalHandler);

        if( 0 != poseidon::mem_sync::KVRegMgr::get_mutable_instance().init(Config::get_mutable_instance().zk_iplist(),
                Config::get_mutable_instance().local_ip(), Config::get_mutable_instance().ds_port()))
        {
            LOG_ERROR("KVRegMgr Init Failed");
            rt = -4;
            break;
        }

        if (!g_watcher.Init())
        {
            LOG_ERROR("DataWatcher Init Failed!");
            rt = -5;
            break;
        }

        LOG_INFO("-----Init Finish-----");
    }while(0);
    return rt;
}

int run()
{
    int rt=0;
    do{
        LOG_INFO("-----DataWatcher Start to run-----");
        rt = g_watcher.Run();
    }while(0);
    return rt;
}

int main(int argc, char* argv[])
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

    int rt = 0;
    do{
        rt = init(conf_file.c_str());
        if(rt != 0)
        {
            fprintf(stderr, "%s init error, return[%d]\n", argv[0], rt);
            break;
        }
        rt = run();
        fprintf(stderr, "%s process will exit, rt[%d]\n", argv[0], rt);

    }while(0);
    return rt;
}



