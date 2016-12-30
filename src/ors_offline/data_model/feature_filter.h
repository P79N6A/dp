#ifndef _ORS_OFFLINE_DATA_MODEL_FEATURE_FILTER_
#define _ORS_OFFLINE_DATA_MODEL_FEATURE_FILTER_

#include <stdint.h>
#include <string>
#include <boost/unordered_set.hpp>
#include "counting_bloom_filter.h"

namespace poseidon {
namespace ors_offline {

//特征过滤器，用于每次判断特征是否加入模型
class FeatureFilter {
public:
    virtual ~FeatureFilter();

    //check whether this feature_id should be use
    virtual bool Check(uint64_t feature_id) = 0;

    //check whether this feature_id should be use and increase its count
    virtual bool AddAndCheck(uint64_t feature_id) = 0;

    //include this feature_id directly
    virtual void Include(uint64_t feature_id) = 0;

    virtual void Clear() = 0;
};

class ProbabilisticFeatureFilter: public FeatureFilter {
public:
    ProbabilisticFeatureFilter(float add_rate);
    bool Check(uint64_t feature_id);
    bool AddAndCheck(uint64_t feature_id);
    void Include(uint64_t feature_id);
    void Clear();
private:
    boost::unordered_set<uint64_t> _feature_set;
    float _add_rate;
};

class CountingBloomFeatureFilter: public FeatureFilter {
public:
    CountingBloomFeatureFilter(int add_count);
    bool Check(uint64_t feature_id);
    bool AddAndCheck(uint64_t feature_id);
    void Include(uint64_t feature_id);
    void Clear();
private:
    CBF::CountingBloomFilter<1000000000, 30000000> _cbf;
    boost::unordered_set<uint64_t> _feature_set;
    float _add_count;
};

}
}

#endif
