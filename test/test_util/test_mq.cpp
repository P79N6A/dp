/**
 **/


#include "util/ipc_mq.h"
#include <iostream>

#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>


int main()
{
    int rt;
    poseidon::util::ipc::MQ mq;
    rt=mq.init(0x123, 0600);
    if(rt != 0)
    {
        printf("mq.init return[%d]\n", rt);
        return -1;
    }
#if 0
    char input[16]="hello";

    rt=mq.push(16, input, strlen(input));
    if(rt != 0)
    {
        printf("mq.push return[%d]\n", rt);
        return -1;
    }
#endif
    long outtype=0;
    char data[8192];
    int size;
    rt=mq.get(0, outtype, data, size );
    if(rt != 0)
    {
        printf("mq.get return[%d]\n", rt);
        return -1;
    }
    printf("outtype[%ld]data[%s]size[%d]\n", outtype, data, size);

    return 0;
}

