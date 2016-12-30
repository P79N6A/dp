/**
 **/
#include "src/mem_sync/mem_manager/mem_manager.h"
#include <stdio.h>

#define LOG_ERR(fmt, a...) fprintf(stderr, "[%d in %s]"fmt, __LINE__, __FILE__, ##a )


int main(int argc, char * argv [])
{
    int rt=0;
    do{
        void * addr;
        rt=MM_INST().update_data(5, 3, 100, addr);
        if(rt != 0)
        {
            LOG_ERR("update_data error[%d]\n", rt);
            break;
        }
        snprintf((char *)addr, 100, "hello %s", "world");
        rt=MM_INST().update_done(5);
        if(rt != 0)
        {
            LOG_ERR("update_data error[%d]\n", rt);
            break;
        }
        const void * addr2;
        uint64_t size2;
        int version;
        rt=MM_INST().get_addr(5, addr2, size2, &version);
        if(rt != 0)
        {
            LOG_ERR("update_data error[%d]\n", rt);
            break;
        }
        printf("addr[%s]size[%lu]version[%d]\n", (const char *)addr2, size2, version);
//        MM_INST().        
    }while(0);
    return rt;
}

