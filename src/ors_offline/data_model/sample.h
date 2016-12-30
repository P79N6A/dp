#ifndef _ORS_OFFLINE_DATA_MODEL_SAMPLE_
#define _ORS_OFFLINE_DATA_MODEL_SAMPLE_

#include <stdint.h>
#include <set>
#include <string>
#include <boost/unordered_map.hpp>
#include "feature_filter.h"

namespace poseidon {
namespace ors_offline {

//样品类，负责生成样品，如样品反序列化，特征id化，信息保存于自身
class Sample {
public:
    virtual ~Sample();

    //将一个字符串反序列化为样品
    virtual int ParseLine(std::string line) = 0;
};

std::istream& operator>>(std::istream& ins, Sample& sample);

class SampleIterator {
public:
    SampleIterator();
    SampleIterator(std::istream& ins, Sample* sample);
    SampleIterator& operator++();
    SampleIterator operator++(int);
    Sample* operator*() const;
    Sample const* operator->() const;
    bool operator==(SampleIterator const& si);
    bool operator!=(SampleIterator const& si);
private:
    std::istream* _ins;
    Sample* _sample;
};

typedef uint64_t (*String2Id)(std::string);

//具体sample声明
class SparseBinaryFeatureLabelSample: public Sample {
public:
    /*
     * line_type: 反序列化方案序号
     * feature_filter: 用于判断特征是否加入模型
     * feature_name_to_id_fun: 特征id化函数
     */
    SparseBinaryFeatureLabelSample(int line_type, FeatureFilter* feature_filter,
            String2Id feature_name_to_id_fun);

    bool Label();
    std::set<uint64_t>* Features();
    int ParseLine(std::string line);

    //序列化
    std::string Display();

    //特征id映射回特征名称
    std::string FeatureId2Name(uint64_t feature_id);
private:
    int parseLineImpl0(std::string line);
    String2Id featureNameToIdFun;
    std::set<uint64_t> _features;
    bool _label;
    int _line_type;
    boost::unordered_map<uint64_t, std::string> _feature_id_name_map;
    FeatureFilter* _feature_filter;
};

uint64_t Cityhash(std::string feature_name);

}
}

#endif
