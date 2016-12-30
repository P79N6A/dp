/**
 **/
#include <stdio.h>
#include <iostream>
#include <sys/wait.h>
#include "../process/control_process.h"
#include "config.h"
#include "ha.h"
#include "util/log.h"
#include "util/func.h"
#include "comm_event.h"
#include "comm_event_interface.h"
#include "comm_event_factory.h"
#include "session_manager.h"
#include "scheduler.h"
#include "util/pvlog.h"
#include "pvlog_trans.h"
#include "yunos_game_map.h"
#include "func.h"
#include "gflags/gflags.h"
#include "api/exp_api.h"

using namespace poseidon;
using namespace poseidon::control;
using namespace dc::common::comm_event;

DEFINE_string(conf, "../etc/conf.cfg", "配置文件路径");
DEFINE_string(pid,"../run.pid","pid文件路径");

void usage(char * basename)
{
    fprintf(stderr, "Usage:%s config_file", basename);
}

/**
 * @brief  初始化环境
 **/
int init(int argc, char * argv [])
{
  google::SetVersionString("1.8.26");
  ::google::ParseCommandLineFlags(&argc, &argv, true);
    int rt=0;
    do{
        /*使用gflags替代
        if(argc < 2)
        {
            usage(argv[0]);
            rt=-1;
            break;
        }*/
        
        /*step 1: 读取配置文件*/
        if(Config::get_mutable_instance().parse(FLAGS_conf.c_str()) != 0)
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

        rt=CommFactoryInterface::instance().init();
        if(rt != 0)
        {
            LOG_ERROR("CommFactoryInterface::instance().init() error, return [%d]", rt);
            rt=-2;
            break;
        }



        /*step 5: 创建多个进程, 让所有进程都监听相同的Adapter端口*/
        int i=0;
        int worker_count=Config::get_mutable_instance().worker_count();
        int pinx=0;   //进程INDEX
        std::map<int, int> proc_info; 
        while(1)
        {
            pid_t pid=fork();
            if(pid >0)
            {
              proc_info[pid]=i;
              if( ++i < worker_count )
              {
                  continue;
              }else
              {
                //写入pid
                //poseidon::util::Func::write_pid_file("../run.pid");
                if(!poseidon::util::Func::single_instance(FLAGS_pid))
                {
                  fprintf(stderr, " %s already run...\n",argv[0]);
                  exit(-1);
                }
                
                //2.等待子进程异常退出，重启进程
                while(1)
                {
                    int status;
                    pid_t pid_exit=wait(&status);
                    if(pid_exit < 0)
                    {
                        LOG_ERROR("wait 异常退出");
                        continue;
                    }
                    pinx=proc_info[pid_exit];
                    LOG_ERROR("子进程退出重启,pindex[%d]pid[%d]", pinx, pid_exit);
                    pid_t pid_new=fork();
                    if(pid_new > 0)
                    {
                        //记录新的进程ID
                        proc_info[pid_new]=pinx;
                        continue;
                    }else if(pid_new == 0)
                    {
                        break;
                    }
                }
                //退出重新创建的子进程
                break;
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
    	if(CommFactoryInterface::instance().re_init() != 0)
        {
        	LOG_ERROR("re_init error\n");
        	return -1;
        }




        /*step 7: init SessionManager*/
        SessionManager::get_mutable_instance().init(Config::get_mutable_instance().max_session());
        SessionManager::get_mutable_instance().set_session_timeout(Config::get_mutable_instance().session_timeout());

        /*step 6: 创建其他Process， 其他Process不能共用端口*/
        /* 各个进程端口的偏移量*/
        int OffPort=Config::get_mutable_instance().off_port()*pinx;

        int AdapterPort=Config::get_mutable_instance().adapter_port()+OffPort;
        int QpPort=Config::get_mutable_instance().qp_port()+OffPort;
        int SnPort=Config::get_mutable_instance().sn_port()+OffPort;
        int DnPort=Config::get_mutable_instance().dn_port()+OffPort;
        int OrsPort=Config::get_mutable_instance().ors_port()+OffPort;
        int FbPort=Config::get_mutable_instance().fb_port()+OffPort;
        
        /*step 4: 创建Adapter实例*/
        ProcessAdapter::get_mutable_instance().bindaddr(
                Config::get_mutable_instance().local_ip(),
                AdapterPort);

        ProcessQp::get_mutable_instance().bindaddr(
                Config::get_mutable_instance().local_ip(),
                QpPort);
        ProcessSn::get_mutable_instance().bindaddr(
                Config::get_mutable_instance().local_ip(),
                SnPort);
        ProcessDn::get_mutable_instance().bindaddr(
                Config::get_mutable_instance().local_ip(),
                DnPort);
        ProcessOrs::get_mutable_instance().bindaddr(
                Config::get_mutable_instance().local_ip(),
                OrsPort);
        ProcessFeedback::get_mutable_instance().bindaddr(
                Config::get_mutable_instance().local_ip(),
                FbPort);

        CommFactoryInterface::instance().add_comm(&(ProcessAdapter::get_mutable_instance()));
        CommFactoryInterface::instance().add_comm(&(ProcessQp::get_mutable_instance()));
        CommFactoryInterface::instance().add_comm(&(ProcessSn::get_mutable_instance()));
        CommFactoryInterface::instance().add_comm(&(ProcessDn::get_mutable_instance()));
        CommFactoryInterface::instance().add_comm(&(ProcessOrs::get_mutable_instance()));
        CommFactoryInterface::instance().add_comm(&(ProcessFeedback::get_mutable_instance()));
        
        int max_qps=Config::get_mutable_instance().max_qps_per_proc();
        LOG_ERROR("config max_qps[%d]\n", max_qps);

        //设置最大的QPS
        Scheduler::get_mutable_instance().set_max_qps(max_qps);

        int nrt=HA_INIT(Config::get_mutable_instance().zk_iplist());
        if(nrt != 0)
        {
            LOG_ERROR("HA_INIT error");
            rt=-1;
            break;
        }
        HA_REG("controler", Config::get_mutable_instance().local_ip(), AdapterPort);

        char pvfilename[256];
        snprintf(pvfilename, 256, "default%d", pinx);
        PV_NAME(pvfilename);

        PvlogTrans::get_mutable_instance().init(Config::get_mutable_instance().pv_trans_ip(), Config::get_mutable_instance().pv_trans_port()); 
        
        //初始化实验系统
        
        bool exp_rt=false;
        if(Config::get_mutable_instance().mem_sync_shm_key()==0)
        {
          exp_rt=exp_sys::ExpApi::get_mutable_instance().init();
        }
        else
        {
          exp_rt=exp_sys::ExpApi::get_mutable_instance().init(Config::get_mutable_instance().mem_sync_shm_key());
        }
        if(!exp_rt)
        {
          LOG_ERROR("exp api init error,exp_rt : %d",exp_rt);
        }
        else
        {
          LOG_INFO("exp api init succ");
        }
        
        //读取yunos映射文件
        if(!YunosGameMap::get_mutable_instance().init(Config::get_mutable_instance().yunos_mapped_path()))
        {
          LOG_ERROR("YunosGameMap init error,path : ");
        }

    }while(0);
    return rt;
}

int run()
{
    int rt=0;
    do{
        CommFactoryInterface::instance().run();
    }while(0);
    return rt;
}

int main(int argc, char * argv [])
{
    int rt=0;
    do{
        rt=init(argc, argv);
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
