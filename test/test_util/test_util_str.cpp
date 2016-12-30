/**
 **/


#include "util/util_str.h"
#include <iostream>


int main()
{
    poseidon::util::UtilStr ustr;
    std::cout<<ustr.format(" %s_%s  ","a_b", "c_d").replace('_', 'N').to_lower().to_upper().trim().str()<<std::endl;
    poseidon::util::UtilStr ustr1("a`b`c`d");
    std::cout<<ustr1.replace('`', '_').str()<<std::endl;
    return 0;
}
