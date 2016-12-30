/**
 **/

#include "util/qps_control.h"

int main(int argc, char * argv[])
{
    poseidon::util::QpsControl q;
    q.set_max_qps(1);
    for(int i=0;i<1000000;i++)
    {
        while(1)
        {
            if(!q.allow())
            {
                continue;
            }
            std::cout<<i<<std::endl;
            break;
        }
    }
    return 0;
}


