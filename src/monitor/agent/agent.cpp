/**
 **/

#include <stdio.h>

#include "config.h"
#include "util/func.h"
#include "util/log.h"
#include "agent_proc.h"

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
    
        rt=AgentProc::get_mutable_instance().init();
        if(rt != 0)
        {
            LOG_ERROR("AgentProc init error[%d]\n", rt);
            break;
        }

    }while(0);
    return rt;
}
void run()
{
    AgentProc::get_mutable_instance().run();
}

int main(int argc, char * argv [])
{
    int rt=0;
    do{
        rt = init(argc, argv);
        if(rt != 0)
        {
            fprintf(stderr, "init return error[%d]\n", rt);
        }
        run();
        fprintf(stderr, "process will exit\n");
    }while(0);
    return rt;
}


