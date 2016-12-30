#include "counting_bloom_filter.h"
#include <stdio.h>
#include "third_party/cityhash/include/city.h"

namespace poseidon {
namespace ors_offline {
namespace CBF {

void CbfInsert(const char* val, size_t val_size, unsigned char* slots,
        size_t bin_num, size_t hash_num) {
    for (size_t i = 0; i < hash_num; ++i) {
        size_t bin_pos = CityHash64WithSeed(val, val_size, i) % bin_num;
        if (slots[bin_pos] < 255) {
            slots[bin_pos]++;
        }
    }
}

bool CbfContain(const char* val, size_t val_size, int count_num,
        unsigned char* slots, size_t bin_num, size_t hash_num) {
    for (size_t i = 0; i < hash_num; ++i) {
        size_t bin_pos = CityHash64WithSeed(val, val_size, i) % bin_num;
        if (slots[bin_pos] < count_num) {
            return false;
        }
    }
    return true;
}

bool CbfInsertAndContain(const char* val, size_t val_size, int count_num,
        unsigned char* slots, size_t bin_num, size_t hash_num) {
    bool is_contain = true;
    for (size_t i = 0; i < hash_num; ++i) {
        size_t bin_pos = CityHash64WithSeed(val, val_size, i) % bin_num;
        if (slots[bin_pos] < 255) {
            slots[bin_pos]++;
        }
        if (slots[bin_pos] < count_num) {
            is_contain = false;
        }
    }
    return is_contain;
}

}
}
}
