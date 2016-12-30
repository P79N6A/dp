/**
 **/
#include "monitor_api.h"
#include <stdio.h>

int main(int argc, char * argv[])
{
    if(argc != 3)
    {
        fprintf(stderr, "%s attr_id value\n", argv[0]);
        return -1;
    }
    int attr_id=atoi(argv[1]);
    long value=strtoul(argv[2], NULL, 0);
    MON_SET(attr_id, value);
    return 0;
}

