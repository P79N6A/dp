/**
 **/
#include "log.h"
#include <iostream>
#include <string.h>

void usage(char * basename)
{
    fprintf(stderr, "%s [FATAL|ERROR|WARN|INFO|DEBUG] msg\n", basename);
}
int main(int argc, char * argv [])
{
    if(argc < 3)
    {
        usage(argv[0]);
        return -1;
    }
#if 1
    if(!LOG_INIT("./log4cpp.conf", "test"))
    {
        std::cerr<<"LOG_INIT error"<<std::endl;
        return -1;
    }
#endif

    if(strcmp(argv[1], "FATAL")==0)
    {
        LOG_FATAL("%s", argv[2]);
    }else if(strcmp(argv[1], "ERROR")==0)
    {
        LOG_ERROR("%s", argv[2]);
    }else if(strcmp(argv[1], "WARN")==0)
    {
        LOG_WARN("%s", argv[2]);
    }else if(strcmp(argv[1], "INFO")==0)
    {
        LOG_INFO("%s", argv[2]);
    }else if(strcmp(argv[1], "DEBUG")==0)
    {
        LOG_DEBUG("%s", argv[2]);
    }else
    {
        usage(argv[0]);
    }
}
