/**
 **/
#include "ha.h"
#include "func.h"
#include <stdio.h>

int main(int argc, char * argv [])
{
    int rt=0;
    do{
//      rt=HA_INIT("10.32.50.180:2181,10.32.50.181:2181,10.32.50.182:2181,10.32.50.212:2181,10.32.50.213:2181");

        if(argc < 3)
        {
            exit(1);
        }
        int port=atoi(argv[1]);
        int quota=atoi(argv[2]);
        rt=HA_INIT("127.0.0.1:2181");
        if(rt != 0)
        {
            fprintf(stderr, "ha_init error");
            break;
        }
        rt=HA_REG_Q("controler", "127.0.0.1", port, quota);
        if(rt != 0)
        {
            fprintf(stderr, "HA_REG error[%d]", rt);
            break;
        }
        sleep(100000);

    }while(0);
    return rt;
}

