#include <iostream>
#include <string.h>
#include "util/func.h"

using namespace std;
using namespace poseidon;

void usage()
{
    cout << "./city_hash str" << endl;
}

int main(int argc, char * argv [])
{
    if (argc != 2)
    {
        usage();
        return -1;
    }

    std::cout << util::Func::BytesHash64(argv[1], strlen(argv[1])) << endl;
    std::cout << util::Func::BytesHash32(argv[1], strlen(argv[1])) << endl;

    return 0;
}
