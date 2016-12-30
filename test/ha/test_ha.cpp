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
        rt=HA_INIT("127.0.0.1:2181");
        if(rt != 0)
        {
            fprintf(stderr, "ha_init error");
            break;
        }
        for(int i=0; i<100;i++)
        {
        struct sockaddr_in addr;
        rt=HA_GET_ADDR("controler", addr);
        if(rt != 0)
        {
            fprintf(stderr, "HA_GET_ADDR error");
            break;
        }
        std::cout<<poseidon::util::Func::to_str(addr)<<std::endl;
        }

    }while(0);
    return rt;
}

