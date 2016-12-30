#include "feature_filter.h"
#include <fstream>
#include <algorithm>

namespace poseidon {
namespace ors_offline {

FeatureFilter::~FeatureFilter() {
}

ProbabilisticFeatureFilter::ProbabilisticFeatureFilter(float add_rate) :
        _add_rate(add_rate) {
    srand((unsigned) time(0));
}

bool ProbabilisticFeatureFilter::Check(uint64_t feature_id) {
    return _feature_set.find(feature_id) != _feature_set.end();
}

bool ProbabilisticFeatureFilter::AddAndCheck(uint64_t feature_id) {
    if (_feature_set.find(feature_id) != _feature_set.end()) {
        return true;
    } else {
        float p = rand() / float(RAND_MAX);
        if (p < _add_rate) {
            _feature_set.insert(feature_id);
            return true;
        } else {
            return false;
        }
    }
}

void ProbabilisticFeatureFilter::Include(uint64_t feature_id) {
    _feature_set.insert(feature_id);
}

void ProbabilisticFeatureFilter::Clear() {
    _feature_set.clear();
}

CountingBloomFeatureFilter::CountingBloomFeatureFilter(int add_count) {
    _add_count = std::min(add_count, 255);
}

bool CountingBloomFeatureFilter::AddAndCheck(uint64_t feature_id) {
    if (_feature_set.find(feature_id) != _feature_set.end()) {
        return true;
    } else {
        if (_cbf.InsertAndContain((char*) &feature_id, sizeof(uint64_t),
                _add_count)) {
            _feature_set.insert(feature_id);
            return true;
        }
    }
    return false;
}

bool CountingBloomFeatureFilter::Check(uint64_t feature_id) {
    return _feature_set.find(feature_id) != _feature_set.end();
}

void CountingBloomFeatureFilter::Include(uint64_t feature_id) {
    _feature_set.insert(feature_id);
}

void CountingBloomFeatureFilter::Clear() {
    _feature_set.clear();
    _cbf.Clear();
}

}
}
