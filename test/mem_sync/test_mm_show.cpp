/**
 **/


#include "src/mem_sync/mem_manager/mem_manager.h"
#include <stdio.h>

#define LOG_ERR(fmt, a...) fprintf(stderr, "[%d in %s]"fmt, __LINE__, __FILE__, ##a )


int main(int argc, char * argv [])
{
    int rt=0;
    do{
        if(argc < 2)
        {
            fprintf(stderr, "%s dataid\n", argv[0]);
            rt=-1;
            break;
        }

        MM_INST().show_data_info(atoi(argv[1]));
    }while(0);
    return rt;
}


