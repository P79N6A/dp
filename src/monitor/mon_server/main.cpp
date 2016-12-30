/**
 **/

#include <stdio.h>

#include "config.h"
#include "util/func.h"
#include "util/log.h"
#include "monitor_listener.h"
#include "monitor_mysql.h"
#include "monitor_tans.h"

using namespace poseidon::monitor;

void usage(const char * basename)
{
    fprintf(stderr, "%s config_file\n", basename);
}


/**
 * @brief  初始化环境
 **/
int init(int argc, char * argv [])
{
    int rt=0;
    do{
        if(argc < 2)
        {
            usage(argv[0]);
            rt=-1;
            break;
        }
        /*step 1: 读取配置文件*/
        if(Config::get_mutable_instance().parse(argv[1]) != 0)
        {
            rt=-2;
            break;
        }

        /*step 2: push process into daemon*/
        poseidon::util::Func::DaemonInit();

        /*step 3: LOG_INIT*/
        if(!LOG_INIT(   Config::get_mutable_instance().log_conf(),
                        Config::get_mutable_instance().log_category()
                     ))
        {
            fprintf(stderr, "LOG_INIT error[%s, %s]\n", Config::get_mutable_instance().log_conf(), Config::get_mutable_instance().log_category());
            rt=-1;
            break;
        }

        LOG_INFO("LOG_INIT SUCCESS!");

        rt=dc::common::comm_event::CommFactoryInterface::instance().init();
        if(rt != 0)
        {
            LOG_ERROR("CommFactoryInterface::instance().init() return error\n");
            break;
        }

        dc::common::comm_event::CommFactoryInterface::instance().add_comm_tcp(&(MonitorListener::get_mutable_instance()));
        MonitorListener::get_mutable_instance().init();

        int ServPort=Config::get_mutable_instance().server_port();
        MonitorListener::get_mutable_instance().listen_on_addr(Config::get_mutable_instance().local_ip(), ServPort, 5);


        /*step 5: 创建多个进程, 让所有进程都监听相同的Net端口*/
        int i=0;
        int worker_count=Config::get_mutable_instance().worker_count();
        int pinx=0;   //进程INDEX
        while(1)
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
                    exit(0);
                }
            }else if(pid == 0)
            {
                pinx=i;
                break;
            }
        }
        /*设置进程ID，表示这是第pidx个进程*/
        Config::get_mutable_instance().process_idx(pinx);

        /*here: father process exit, and run in child process*/
    	if(dc::common::comm_event::CommFactoryInterface::instance().re_init() != 0)
        {
        	LOG_ERROR("re_init error\n");
            rt=-1;
            break;
        }

        rt=MonitorMysql::get_mutable_instance().init(
                Config::get_mutable_instance().mysql_host(),
                Config::get_mutable_instance().mysql_user(),
                Config::get_mutable_instance().mysql_pass());
        if(rt != 0)
        {
            LOG_ERROR("MonitorMysql init error[%d]\n", rt);
            rt=-1;
            break;
        }

        MonitorTans::get_mutable_instance().init(Config::get_mutable_instance().tans_ip(), Config::get_mutable_instance().tans_port());

    }while(0);
    return rt;
}
void run()
{
    dc::common::comm_event::CommFactoryInterface::instance().run();
}

int main(int argc, char * argv [])
{
    int rt=0;
    do{
        rt = init(argc, argv);
        if(rt != 0)
        {
            fprintf(stderr, "init return error[%d]\n", rt);
            break;
        }
        run();
        fprintf(stderr, "process will exit\n");
    }while(0);
    return rt;
}


