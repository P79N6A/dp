/**
**/

#ifndef _MODEL_UPDATER_API_LR_MODEL_PREDICTOR_H_
#define _MODEL_UPDATER_API_LR_MODEL_PREDICTOR_H_
//include STD C/C++ head files
#include <math.h>

//include third_party_lib head files
#include "third_party/boost/include/boost/serialization/singleton.hpp"
#include "src/model_updater/api/algo_model_data_api.h"
#include "util/func.h"

namespace poseidon
{
namespace model_updater
{
class LRModelPredictor : public boost::serialization::singleton<LRModelPredictor>
{

public:
    LRModelPredictor();
    virtual ~LRModelPredictor();

    bool SelectModel(uint32_t model_id)
    {
        if (!AlgoModelDataApi::get_mutable_instance().GetLRModelMeta(model_id, &m_lr_model_meta))
        {
            return false;
        }

        m_model_id = model_id;
        m_fea_val_idx = 0;
        m_trans_fea_map.clear();
        for (int i = 0; i < m_lr_model_meta.transform_fea_vals_size(); i++)
        {
            const ors::LRModel::TransformFeaVal& val = m_lr_model_meta.transform_fea_vals(i);
            m_trans_fea_map[val.raw_fea() + "_" + val.raw_val()] = std::make_pair(val.trans_fea(), val.trans_val());
        }
        return true;
    }

    void SetFeaVal(const std::string& fea, const std::string& val)
    {
        std::string fv = fea + "_" + val;
        if (val.empty())
        {
            fv = fea + "_0";
        }
        m_fea_val_map[fea] = std::make_pair(m_fea_val_idx, m_fea_val_idx);
        m_fea_val_vec[m_fea_val_idx++] = fv;

        std::map<std::string, std::pair<std::string, std::string> >::iterator iter = m_trans_fea_map.find(fv);
        if (iter != m_trans_fea_map.end())
        {
            fv = iter->second.first + "_" + iter->second.second;
            m_fea_val_map[iter->second.first] = std::make_pair(m_fea_val_idx, m_fea_val_idx);
            m_fea_val_vec[m_fea_val_idx++] = fv;
        }
    }

    void SetFeaVal(const std::string& fea, int val)
    {
        return SetFeaVal(fea, util::Func::to_str(val));
    }

    void SetFeaVal(const std::string& fea, const std::vector<std::string>& vals)
    {
        if (vals.size() == 0)
        {
            SetFeaVal(fea, "");
            return;
        }
        m_fea_val_map[fea] = std::make_pair(m_fea_val_idx, m_fea_val_idx + vals.size() - 1); 
        for (size_t i = 0; i < vals.size(); i++)
        {
            std::string fv = fea + "_" + vals[i];
            m_fea_val_vec[m_fea_val_idx++] = fv;
        }
    }


    float CalPredictScore();
protected:
    float Sigmoid(float x)
    {
        return 1 / (1 + exp(-x));
    }
protected:
    ors::LRModel::Meta m_lr_model_meta;
    std::map<std::string, std::pair<int,int> > m_fea_val_map;
    std::vector<std::string> m_fea_val_vec;
    int m_fea_val_idx;
    uint32_t m_model_id;
    std::map<std::string, std::pair<std::string, std::string> > m_trans_fea_map;

};

#define LR_MODEL_SETUP(model_id) poseidon::model_updater::LRModelPredictor::get_mutable_instance().SelectModel(model_id)
#define LR_MODEL_FEA_VAL(fea, val) poseidon::model_updater::LRModelPredictor::get_mutable_instance().SetFeaVal(fea, val)
#define LR_MODEL_SCORE() poseidon::model_updater::LRModelPredictor::get_mutable_instance().CalPredictScore()

} // namespace model_updater
} // namespace poseidon

#endif // _MODEL_UPDATER_API_LR_MODEL_PREDICTOR_H_

