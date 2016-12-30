#include <iostream>
#include "src/ors/common/pro_stat.h"
#include "util/func.h"

using namespace std;
using namespace poseidon;
using namespace poseidon::ors;

void usage()
{
    cout << "./property_look property_type property_id" << endl;
}

int main(int argc, char * argv [])
{
    if (argc != 3)
    {
        usage();
        return -1;
    }

    uint8_t pro_type = atoi(argv[1]);
    uint64_t pro_id = atoi(argv[2]);
    if (pro_id == 0)
    {
        pro_id = util::Func::BytesHash64(argv[2], strlen(argv[2]));
    }

    TPropertyKey key;
    key.property_type = pro_type;
    key.property_id = pro_id;

    TPropertyValue* value;
    poseidon::ors::ProStat::get_mutable_instance().PRO_ON();
    if (poseidon::ors::ProStat::get_mutable_instance().GetPropertyStat(key, &value))
    {
        std::cout << key.to_string() << endl;
        std::cout << value->to_string() << endl;
        return 0;
    }

    std::cout << key.to_string() << " NO FOUND!" << endl;
    return -1;
}
