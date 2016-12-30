#include "model.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "util/log.h"
#include "src/ors_offline/common/utility.h"
#include "util.h"

namespace poseidon {
namespace ors_offline {

struct LabelAndPred {
    bool label;
    float pred;
    LabelAndPred(bool label, float pred) {
        this->label = label;
        this->pred = pred;
    }
};

struct PosAndNeg {
    float pos_num;
    float neg_num;
    PosAndNeg() {
        pos_num = 0;
        neg_num = 0;
    }
};

bool lp_less(const LabelAndPred &i, const LabelAndPred &j) {
    return i.pred < j.pred;
}

Model::Model(uint64_t global_id) {
    this->global_id = global_id;
}

Model::~Model() {
}

int Model::CalAuc(std::string path, SparseBinaryFeatureLabelSample* sample,
        float &auc, float& avg_ctr, float &p_ctr) {
    using namespace std;
    vector<string> files;
    listFiles(path, files);
    if (files.empty()) {
        LOG_ERROR("no train files in path: %s", path.c_str());
        return -1;
    }

    std::vector<LabelAndPred> lp_vec;
    float pos_num = 0, neg_num = 0, pred, pred_sum = 0;
    bool label;

    for (vector<string>::iterator it = files.begin(); it != files.end(); it++) {
        LOG_INFO("Test file: %s", (*it).c_str());
        ifstream test_ins((*it).c_str());
        if (!test_ins.good()) {
            LOG_ERROR("Fail to load test set: %s", (*it).c_str());
            return -1;
        }

        while (test_ins >> *sample) {
            pred = this->PredictOne(sample);
            pred_sum += pred;
            label = sample->Label();
            if (label) {
                pos_num++;
            } else {
                neg_num++;
            }
            lp_vec.push_back(LabelAndPred(label, pred));
        }
        test_ins.close();
    }

    avg_ctr = pos_num / (pos_num + neg_num);
    p_ctr = pred_sum / (pos_num + neg_num);
    LOG_INFO("Real avg-ctr: %f, Predicted avg-ctr: %f", avg_ctr, p_ctr);

    std::sort(lp_vec.begin(), lp_vec.end(), lp_less);
    float rank_acc = 0.0;
    for (size_t i = 0; i < lp_vec.size(); i++) {
        if (lp_vec[i].label) {
            rank_acc += i + 1;
        }
    }

    auc = (rank_acc - pos_num * (pos_num + 1) / 2) / pos_num / neg_num;
    return 0;
}

//FTRL model
FtrlModel::FtrlModel(ModelParams& model_params,
        util::HashShm<uint64_t, FeatureParams>* shm, uint64_t global_id) :
        Model(global_id) {
    _model_params = model_params;
    _shm = shm;
    _fix_feature_set = NULL;
}

FtrlModel::~FtrlModel() {
    _shm->Detach();
}

int FtrlModel::InitStrongFeatureWeights(
        boost::unordered_set<uint64_t> strong_feature_id, std::string path,
        SparseBinaryFeatureLabelSample* sample) {
    using namespace std;
    vector<string> files;
    listFiles(path, files);
    if (files.empty()) {
        LOG_ERROR("no train files in path: %s", path.c_str());
    }

    // stat pos and neg for strong feature id
    boost::unordered_map<uint64_t, PosAndNeg> pos_neg_stat_map;
    float pos_all = 0, neg_all = 0;

    for (vector<string>::iterator it = files.begin(); it != files.end(); it++) {
        LOG_INFO("Stat file: %s", (*it).c_str());
        ifstream train_ins((*it).c_str());
        if (!train_ins.good()) {
            LOG_ERROR("Fail to load test set: %s", (*it).c_str());
            return -1;
        }

        while (train_ins >> *sample) {
            if (sample->Label()) {
                pos_all++;
            } else {
                neg_all++;
            }
            if (global_id > 0) {
                if (sample->Label()) {
                    pos_neg_stat_map[global_id].pos_num++;
                } else {
                    pos_neg_stat_map[global_id].neg_num++;
                }
            } else {
                std::set<uint64_t>* fs = sample->Features();
                for (std::set<uint64_t>::iterator it = fs->begin();
                        it != fs->end(); it++) {
                    if (strong_feature_id.find(*it)
                            == strong_feature_id.end()) {
                        continue;
                    }
                    if (sample->Label()) {
                        pos_neg_stat_map[*it].pos_num++;
                    } else {
                        pos_neg_stat_map[*it].neg_num++;
                    }
                }
            }
        }
        train_ins.close();
    }

    float avg_pos_rate = pos_all / (pos_all + neg_all);
    float effective_count = 10.0 / avg_pos_rate; //number of samples to get 10 pos
    for (boost::unordered_map<uint64_t, PosAndNeg>::iterator it =
            pos_neg_stat_map.begin(); it != pos_neg_stat_map.end(); it++) {
        float w;
        float pos_num = it->second.pos_num;
        float neg_num = it->second.neg_num;
        float sample_num = pos_num + neg_num;
        if (sample_num > effective_count) {
            if (0 == pos_num) {
                w = -10;
            } else if (0 == neg_num) {
                w = 0;
            } else {
                // w = std::min(unsigmoid(pos_num / sample_num), -1.0f);
                w = unsigmoid(pos_num / sample_num);
            }
        } else {
            float weighted_rate = ((effective_count - sample_num) * avg_pos_rate
                    + pos_num) / effective_count;
            // w = std::min(unsigmoid(weighted_rate), -1.0f);
            w = unsigmoid(weighted_rate);
        }
        FeatureParams* fps_ptr;
        if (!_shm->Get(it->first, &fps_ptr)) {
            FeatureParams fps;
            _shm->Set(it->first, fps);
        }
        _shm->Get(it->first, &fps_ptr);
        fps_ptr->zi = signum(w) * _model_params.lambda1
                - w
                        * (_model_params.beta / _model_params.alpha
                                + _model_params.lambda2);
        fps_ptr->wi = w;
    }
    return 0;
}

int FtrlModel::Train(std::string path, SparseBinaryFeatureLabelSample* sample) {
    using namespace std;
    vector<string> files;
    listFiles(path, files);
    if (files.empty()) {
        LOG_ERROR("no train files in path: %s", path.c_str());
    }
    int train_num = 0;
    for (vector<string>::iterator it = files.begin(); it != files.end(); it++) {
        LOG_INFO("Train file: %s", (*it).c_str());
        ifstream train_ins((*it).c_str());
        if (!train_ins.good()) {
            LOG_ERROR("Fail to load training set: %s", (*it).c_str());
            return -1;
        }
        while (train_ins >> *sample) {
            train_num++;
            TrainOne(sample);
            if (train_num % 100000 == 0) {
                std::cout << "Trained " << train_num << " samples" << std::endl;
            }
        }

        train_ins.close();
    }
    return 0;
}

int FtrlModel::TrainOne(SparseBinaryFeatureLabelSample* sample) {

    float pred = PredictOne(sample);
    float gi = pred - sample->Label();
    float gi2 = gi * gi;

    std::set<uint64_t>* features = sample->Features();
    if (global_id > 0) {
        features->insert(global_id);
    }
    for (std::set<uint64_t>::iterator it = features->begin();
            it != features->end(); it++) {
        //not to change fix feature
        if (NULL != _fix_feature_set
                && _fix_feature_set->find(*it) != _fix_feature_set->end()) {
            continue;
        }

        FeatureParams* fps_ptr;
        if (!_shm->Get(*it, &fps_ptr)) {
            FeatureParams fps;
            _shm->Set(*it, fps);
        }
        _shm->Get(*it, &fps_ptr);
        float sigma = (sqrt(fps_ptr->ni + gi2) - sqrt(fps_ptr->ni))
                / _model_params.alpha;
        fps_ptr->zi += gi - sigma * fps_ptr->wi;
        fps_ptr->ni += gi2;
        fps_ptr->wi = calFeatureWeight(fps_ptr);
    }
    return 0;
}

float FtrlModel::PredictOne(SparseBinaryFeatureLabelSample* sample) {
    float r = 0;
    std::set<uint64_t>* features = sample->Features();
    if (global_id > 0) {
        features->insert(global_id);
    }
    for (std::set<uint64_t>::iterator it = features->begin();
            it != features->end(); it++) {
        FeatureParams* fps_ptr;
        if (_shm->Get(*it, &fps_ptr)) {
            r += fps_ptr->wi;
        }
    }

    return sigmoid(r);
}

float FtrlModel::calFeatureWeight(FeatureParams* ps) {
    float wi = 0.0;
    if (std::abs(ps->zi) >= _model_params.lambda1) {
        wi = (signum(ps->zi) * _model_params.lambda1 - ps->zi)
                / (_model_params.lambda2
                        + (_model_params.beta + sqrt(ps->ni))
                                / _model_params.alpha);
    }
    return wi;
}

void FtrlModel::RemoveTrivialFeatures() {
    _shm->GotoFirst();
    uint64_t* k;
    FeatureParams* v;
    while (_shm->Walk(&k, &v)) {
        if (0 == v->wi) {
            _shm->Delete(*k);
        }
    }
}

int FtrlModel::SaveModelToFile(std::string model_path) {
    using namespace std;
    ofstream fout(model_path.c_str(), ofstream::out | ofstream::trunc);
    if (!fout.good()) {
        LOG_ERROR("Fail to save model to file: %s", model_path.c_str());
        return -1;
    }

    string sep = "`";
    _shm->GotoFirst();
    uint64_t* k;
    FeatureParams* v;
    while (_shm->Walk(&k, &v)) {
        fout << *k << sep << v->ni << sep << v->zi << sep << v->wi << endl;
    }

    fout.close();
    return 0;
}

int FtrlModel::SaveModelToFile(std::string model_path,
        SparseBinaryFeatureLabelSample* sample) {
    using namespace std;
    ofstream fout(model_path.c_str(), ofstream::out | ofstream::trunc);
    if (!fout.good()) {
        LOG_ERROR("Fail to save model to file: %s", model_path.c_str());
        return -1;
    }

    string sep = "`";
    _shm->GotoFirst();
    uint64_t* k;
    FeatureParams* v;
    while (_shm->Walk(&k, &v)) {
        fout << sample->FeatureId2Name(*k) << sep << v->ni << sep << v->zi
                << sep << v->wi << endl;
    }

    fout.close();
    return 0;
}

int FtrlModel::LoadModelFromFile(std::string model_path) {
    using namespace std;
    _shm->Clear();
    ifstream fin(model_path.c_str(), ifstream::in);
    if (!fin.good()) {
        LOG_ERROR("Fail to load model from file: %s", model_path.c_str());
        return -1;
    }

    string line;
    while (std::getline(fin, line)) {
        vector<string> vec = split(line, "`");
        if (vec.size() != 4) {
            LOG_ERROR("Model file's format is incorrect: %s", line.c_str());
            return -1;
        }

        uint64_t fid;
        istringstream(vec[0]) >> fid;
        FeatureParams fps;
        istringstream(vec[1]) >> fps.ni;
        istringstream(vec[2]) >> fps.zi;
        istringstream(vec[3]) >> fps.wi;
        _shm->Set(fid, fps);
    }

    fin.close();
    return 0;
}

void FtrlModel::SetFixFeatures(boost::unordered_set<uint64_t>* set) {
    _fix_feature_set = set;
}

FtrlModel::FeatureParams::FeatureParams() {
    ni = 0;
    zi = 0;
    wi = 0;
}

FtrlModel::ModelParams::ModelParams() {
    alpha = 0.1;
    beta = 1.0;
    lambda1 = 0.1;
    lambda2 = 1.0;
}

FtrlModel::ModelParams::ModelParams(const FtrlModelConfig_ModelParams& config) {
    alpha = config.alpha();
    beta = config.beta();
    lambda1 = config.lambda1();
    lambda2 = config.lambda2();
}

}
}
