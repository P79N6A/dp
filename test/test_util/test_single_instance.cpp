/**
 **/


#include "util/func.h"
#include <iostream>

#include <stdio.h>
#include <unistd.h>


int main()
{
    if(!poseidon::util::Func::single_instance("./my.pid"))
    {
        printf("process is runing...");
        return -1;
    }
    
    sleep(3600);

    return 0;
}
