/**
 **/
#include "src/mem_sync/mem_manager/mem_manager.h"
#include <stdio.h>
#include <string.h>

#define LOG_ERR(fmt, a...) fprintf(stderr, "[%d in %s]"fmt, __LINE__, __FILE__, ##a )


int main(int argc, char * argv [])
{
    int rt=0;
    do{
        if(argc < 4)
        {
            fprintf(stderr, "%s dataid version data\n", argv[0]);
            rt=-1;
            break;
        }
        int dataid=atoi(argv[1]);
        int version=atoi(argv[2]);
        int datalen=strlen(argv[3]);

        void * addr;
        rt=MM_INST().update_data(dataid, version, datalen, addr);
        if(rt != 0)
        {
            LOG_ERR("update_data error[%d]\n", rt);
            break;
        }
        memcpy(addr, argv[3], datalen);
        rt=MM_INST().update_done(dataid);
        if(rt != 0)
        {
            LOG_ERR("update_data error[%d]\n", rt);
            break;
        }
    }while(0);
    return rt;
}

