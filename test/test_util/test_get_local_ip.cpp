/**
 **/
#include "util/func.h"

int main(int argc, char * argv [])
{
    std::string ip;
    poseidon::util::Func::get_local_ip(ip);
    std::cout<<ip<<std::endl;
    return 0;
}
