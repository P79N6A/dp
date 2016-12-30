#include "sample.h"
#include <stdlib.h>
#include <sstream>
#include <fstream>
#include "third_party/cityhash/include/city.h"
#include "src/ors_offline/common/utility.h"

namespace poseidon {
namespace ors_offline {

std::istream& operator>>(std::istream& ins, Sample& sample) {
    std::string line;
    getline(ins, line);
    sample.ParseLine(line);
    return ins;
}

SampleIterator::SampleIterator() :
        _ins(NULL), _sample(NULL) {
}

SampleIterator::SampleIterator(std::istream& ins, Sample* sample) {
    if (ins.good()) {
        _ins = &ins;
    } else {
        _ins = NULL;
    }
    _sample = sample;
    ++(*this);
}

SampleIterator& SampleIterator::operator++() {
    if (_ins) {
        if (!((*_ins) >> *_sample)) {
            _ins = NULL;
        }
    }
    return *this;
}

SampleIterator SampleIterator::operator++(int) {
    SampleIterator tmp(*this);
    ++(*this);
    return tmp;
}

Sample* SampleIterator::operator*() const {
    return _sample;
}

Sample const* SampleIterator::operator->() const {
    return _sample;
}

bool SampleIterator::operator==(SampleIterator const& si) {
    return ((this == &si) || ((this->_ins == NULL) && (si._ins == NULL)));
}

bool SampleIterator::operator!=(SampleIterator const& si) {
    return !((*this) == si);
}

Sample::~Sample() {

}

//具体sample实现
SparseBinaryFeatureLabelSample::SparseBinaryFeatureLabelSample(int line_type,
        FeatureFilter* feature_filter, String2Id feature_name_to_id_fun) {
    _line_type = line_type;
    _feature_filter = feature_filter;
    featureNameToIdFun = feature_name_to_id_fun;
}

bool SparseBinaryFeatureLabelSample::Label() {
    return _label;
}

std::set<uint64_t>* SparseBinaryFeatureLabelSample::Features() {
    return &_features;
}

int SparseBinaryFeatureLabelSample::ParseLine(std::string line) {
    if (0 == _line_type) {
        return parseLineImpl0(line);
    }
    return -1;
}

std::string SparseBinaryFeatureLabelSample::Display() {
    std::string result = "";
    for (std::set<uint64_t>::iterator it = _features.begin();
            it != _features.end(); it++) {
        if (result != "") {
            result += "\001";
        }

        if (NULL == featureNameToIdFun) {
            std::ostringstream ss;
            ss << *it;
            result += ss.str();
        } else {
            result += _feature_id_name_map[*it];
        }
    }
    std::ostringstream ss;
    ss << _label;
    result += "`" + ss.str();

    return result;
}

std::string SparseBinaryFeatureLabelSample::FeatureId2Name(
        uint64_t feature_id) {
    return _feature_id_name_map[feature_id];
}

int SparseBinaryFeatureLabelSample::parseLineImpl0(std::string line) {
    using namespace std;

    stringstream lineStream(line);
    string cell;
    _features.clear();

    std::vector<string> terms = split(line, "`");
    if (terms.size() < 2) {
        return -1;
    }

    string lstr = terms[0];
    string fstr = terms[1];

    istringstream(lstr) >> _label;
    stringstream fstrStream(fstr);
    uint64_t feature;
    while (getline(fstrStream, cell, '\001')) {
        //index feature_name
        if (featureNameToIdFun != NULL) {
            feature = featureNameToIdFun(cell);
            //filter rare features
            if (_feature_filter != NULL
                    && !_feature_filter->AddAndCheck(feature)) {
                continue;
            }
            _feature_id_name_map[feature] = cell;
        } else {
            istringstream(cell) >> feature;
            //filter rare features
            if (_feature_filter != NULL
                    && !_feature_filter->AddAndCheck(feature)) {
                continue;
            }
        }
        _features.insert(feature);
    }
    return 0;
}

uint64_t Cityhash(std::string feature_name) {
    return CityHash64(feature_name.c_str(), feature_name.size());
}

}
}
