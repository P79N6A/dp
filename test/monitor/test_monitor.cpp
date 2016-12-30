/**
 **/
#include "monitor_api.h"
#include <iostream>
#include <string.h>
#include <stdio.h>

int main(int argc, char * argv [])
{
    if( argc < 5 )
    {
        fprintf(stderr, "usage:%s [add|set] attr val cnt\n", argv[0]);
        return 0;
    }
    int attr=atoi(argv[2]);
    int value=atoi(argv[3]);
    int cnt=atoi(argv[4]);

    if(strcmp(argv[1], "add")==0)
    {
        for(int i=0;i<cnt;i++)
            MON_ADD(attr, value);
    }else if(strcmp(argv[1], "set")==0)
    {
        for(int i=0;i<cnt;i++)
            MON_SET(attr, value);
    }else
    {
        fprintf(stderr, "usage:%s [add|set] attr val cnt\n", argv[0]);
        return 0;
    }

    return 0;
}
