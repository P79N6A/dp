/**
 **/
#include "src/mem_sync/mem_manager/mem_manager.h"
#include <stdio.h>
#include "util/hexdump.h" 

#define LOG_ERR(fmt, a...) fprintf(stderr, "[%d in %s]"fmt, __LINE__, __FILE__, ##a )


int main(int argc, char * argv [])
{
    int rt=0;
    do{
        if(argc < 2)
        {
            fprintf(stderr, "usage:%s dataid\n", argv[0]);
            rt=-1;
            break;
        }
        int dataid=atoi(argv[1]);

        while(1)
        {
            const void * addr;
            uint64_t size;
            int version;
            rt=MM_INST().get_addr(dataid, addr, size, &version);
            if(rt != 0)
            {
                LOG_ERR("update_data error[%d]\n", rt);
                break;
            }
            printf("dataid[%d]size[%ld]version[%d]\n", dataid, size, version );
//            *(char *)addr='a';//coredump，不允许写入
            size=size>4096?4096:size;
            HEXDUMP(addr, (int)size, printf);
            sleep(1);
        }

//        MM_INST().        
    }while(0);
    return rt;
}

