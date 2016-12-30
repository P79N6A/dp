/**
**/

//include STD C/C++ head files


//include third_party_lib head files
#include "src/model_updater/api/lr_model_predictor.h"
#include "util/log.h"

namespace poseidon
{
namespace model_updater
{

LRModelPredictor::LRModelPredictor()
{
    m_fea_val_vec.resize(1024);
    m_fea_val_idx = 0;
    m_model_id = 0;
}

LRModelPredictor::~LRModelPredictor()
{

}

float LRModelPredictor::CalPredictScore()
{
    float weight_sum = 0.0f;
    LRKey key;
    LRValue* value = NULL;

    for (int i = 0; i < m_lr_model_meta.cross_feas_size(); i++)
    {
        std::vector<std::string> fea_vec;
        fea_vec.push_back("");
        for(int j = 0; j < m_lr_model_meta.cross_feas(i).feas_size(); j++)
        {
            const std::string& fea = m_lr_model_meta.cross_feas(i).feas(j);
            std::map<std::string, std::pair<int,int> >::iterator iter = m_fea_val_map.find(fea);
            if (iter == m_fea_val_map.end())
            {
                fea_vec.clear();
                break;
            }
            int dis = iter->second.second - iter->second.first;
            if (dis == 0)
            {
                for (size_t k = 0; k < fea_vec.size(); k++)
                {
                    fea_vec[k] += (m_fea_val_vec[iter->second.first] + "&");
                }
            }
            else 
            {
                std::vector<std::string> copy_fea_vec(fea_vec);
                for (int k = 0; k < dis; k++)
                {
                    fea_vec.insert(fea_vec.end(), copy_fea_vec.begin(), copy_fea_vec.end());
                }

                for (int k = 0; k <= dis; k++)
                {
                    for (size_t w = 0; w < copy_fea_vec.size(); w++)
                    {
                        fea_vec[k * copy_fea_vec.size() + w] += (m_fea_val_vec[iter->second.first + k] + "&");
                    }
                }
            }
        }
        
        for (size_t j = 0; j < fea_vec.size(); j++) 
        {
            key.fea_hash = util::Func::BytesHash64(fea_vec[j].data(), fea_vec[j].size() - 1);
            if (!AlgoModelDataApi::get_mutable_instance().GetLRModelValue(m_model_id, key, &value))
            {
                LOG_DEBUG("fea=%s, fea_hash=%lu, lr model get none", fea_vec[j].c_str(), key.fea_hash);
                continue;
            }

            weight_sum += value->weight; 
            LOG_DEBUG("fea=%s, fea_hash=%lu, lr model get weight=%f", fea_vec[j].c_str(), key.fea_hash, value->weight);
        }
    }

    float score = Sigmoid(weight_sum);
    float norm_score = score / (score + (1 - score) / m_lr_model_meta.negative_down_sampling_rate());
    LOG_DEBUG("model_id=%d, weight_sum=%f, score=%f, negative_down_sampling_rate=%f, norm_score=%f", 
            m_lr_model_meta.model_id(), weight_sum, score, m_lr_model_meta.negative_down_sampling_rate(), norm_score);

    return norm_score;
}



} // namespace model_updater
} // namespace poseidon

