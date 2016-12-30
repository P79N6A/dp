#ifndef _ORS_OFFLINE_FTRL_MODEL_MODEL_
#define _ORS_OFFLINE_FTRL_MODEL_MODEL_

#include <stdint.h>
#include <string>
#include <boost/unordered_set.hpp>
#include "protocol/src/poseidon_proto.h"
#include "util/hash_shm.h"
#include "src/ors_offline/data_model/sample.h"

namespace poseidon {
namespace ors_offline {

//模型类，负责训练模型
class Model {
public:
    //global_id为全局特征id
    Model(uint64_t global_id = 0);
    virtual ~Model();

    /**
     * 批量训练
     * path: 训练集路径
     * sample：样品对象，提供反序列化能力
     */
    virtual int Train(std::string path,
            SparseBinaryFeatureLabelSample* sample) = 0;

    /**
     * 训练单个特征
     * sample: 已反序列化的样品
     */
    virtual int TrainOne(SparseBinaryFeatureLabelSample* sample) = 0;
    virtual float PredictOne(SparseBinaryFeatureLabelSample* sample) = 0;

    /**
     * 计算AUC和pCtr
     * path: 测试集路径
     * sample：样品对象，提供反序列化能力
     */
    int CalAuc(std::string path, SparseBinaryFeatureLabelSample* sample,
            float &auc, float &avg_ctr, float &p_ctr);
    virtual int SaveModelToFile(std::string model_path) = 0;
    virtual int LoadModelFromFile(std::string model_path) = 0;
    uint64_t global_id; //全局公共特征，默认为0即不使用
};

class LrSgdModel: public Model {
public:
    int TrainOne(SparseBinaryFeatureLabelSample* sample);
    float PredictOne(SparseBinaryFeatureLabelSample* sample);
};

class LrBfgsModel: public Model {
public:
    int TrainOne(SparseBinaryFeatureLabelSample* sample);
    float PredictOne(SparseBinaryFeatureLabelSample* sample);
};

class FtrlModel: public Model {
public:
    struct ModelParams {
        float alpha;
        float beta;
        float lambda1;
        float lambda2;
        ModelParams();
        ModelParams(const FtrlModelConfig_ModelParams& config);
    };
    struct FeatureParams {
        float ni;
        float zi;
        float wi;
        FeatureParams();
    };

    FtrlModel(ModelParams& model_params,
            util::HashShm<uint64_t, FeatureParams>* shm, uint64_t global_id);
    ~FtrlModel();

    //根据训练数据初始化强特征权重
    int InitStrongFeatureWeights(
            boost::unordered_set<uint64_t> strong_feature_id, std::string path,
            SparseBinaryFeatureLabelSample* sample);
    int Train(std::string path, SparseBinaryFeatureLabelSample* sample);
    int TrainOne(SparseBinaryFeatureLabelSample* sample);
    float PredictOne(SparseBinaryFeatureLabelSample* sample);
    int SaveModelToFile(std::string model_path);

    //保存模型到文件，提供的sample参数用于将特征id映射会特征名
    int SaveModelToFile(std::string model_path,
            SparseBinaryFeatureLabelSample* sample);
    int LoadModelFromFile(std::string model_path);

    //训练结束之后，去除权重为0的特征
    void RemoveTrivialFeatures();

    //训练中固定强特征权重
    void SetFixFeatures(boost::unordered_set<uint64_t>* set);
private:
    ModelParams _model_params;
    // boost::unordered_map<uint64_t, FeatureParams*> _feature_map;
    float calFeatureWeight(FeatureParams* params);
    util::HashShm<uint64_t, FeatureParams>* _shm;
    boost::unordered_set<uint64_t>* _fix_feature_set;
};

}
}

#endif
