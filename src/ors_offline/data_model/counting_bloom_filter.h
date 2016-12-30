#ifndef ORS_OFFLINE_DATA_MODEL_COUNTING_BLOOM_FILTER_H_
#define ORS_OFFLINE_DATA_MODEL_COUNTING_BLOOM_FILTER_H_

#include <cstring>
#include <cmath>
#include <string>

namespace poseidon {
namespace ors_offline {
namespace CBF {

template<size_t BinNum, size_t StroeNum>
class CountingBloomFilter {
    static const size_t bin_num = BinNum;
    static const size_t hash_num =
            0.7 * BinNum / StroeNum > 1 ? (int) (0.7 * BinNum / StroeNum) : 1;
public:
    CountingBloomFilter() {
        Clear();
    }
    void Insert(std::string val) {
        CbfInsert(val.data(), val.size(), _slots, bin_num, hash_num);
    }
    void Insert(const char* val, size_t val_size) {
        CbfInsert(val, val_size, _slots, bin_num, hash_num);
    }
    bool Contain(std::string val, int count_num) {
        return CbfContain(val.data(), val.size(), count_num, _slots, bin_num,
                hash_num);
    }
    bool Contain(const char* val, size_t val_size, int count_num) {
        return CbfContain(val, val_size, count_num, _slots, bin_num, hash_num);
    }
    bool InsertAndContain(std::string val, int count_num) {
        return CbfContain(val.data(), val.size(), count_num, _slots, bin_num,
                hash_num);
    }
    bool InsertAndContain(const char* val, size_t val_size, int count_num) {
        return CbfInsertAndContain(val, val_size, count_num, _slots, bin_num,
                hash_num);
    }
    void Clear() {
        memset(_slots, 0, bin_num * sizeof(char));
    }
private:
    unsigned char _slots[BinNum];

};

void CbfInsert(const char* val, size_t val_size, unsigned char* slots,
        size_t bin_num, size_t hash_num);
bool CbfContain(const char* val, size_t val_size, int count_num,
        unsigned char* slots, size_t bin_num, size_t hash_num);
bool CbfInsertAndContain(const char* val, size_t val_size, int count_num,
        unsigned char* slots, size_t bin_num, size_t hash_num);

}
}
}

#endif /* COUNTING_BLOOM_FILTER_H_ */
