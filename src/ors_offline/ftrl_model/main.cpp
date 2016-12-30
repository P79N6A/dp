#include <fstream>
#include "gflags/gflags.h"
#include "protocol/src/poseidon_proto.h"
#include "util/log.h"
#include "util/shm.h"
#include "util/proto_helper.h"
#include "src/ors_offline/common/utility.h"
#include "src/ors_offline/data_model/sample.h"
#include "src/ors_offline/data_model/feature_filter.h"
#include "model.h"
#include "util.h"

using namespace std;
using namespace poseidon::ors_offline;
using namespace poseidon::util;
using namespace poseidon::util::shm;

int loadConfig(const char* config_path, FtrlModelConfig& config);
int load_model_from_file_to_filter(std::string model_path,
        FeatureFilter* feature_filter);
int predict_test(Model& model, SparseBinaryFeatureLabelSample* sample,
        string test_path, string predict_path);
int load_model_from_mem_to_filter(
        HashShm<uint64_t, FtrlModel::FeatureParams>& shm,
        FeatureFilter* feature_filter);
int load_strong_feature_from_file(std::string strong_feature_path,
        boost::unordered_set<uint64_t>& strong_feature_set,
        String2Id feature_name_to_id_fun);

DEFINE_string(conf, "./conf/ftrl_model.conf", "配置文件路径");
DEFINE_string(train_path, "", "训练集路径，支持文件夹，为空则跳过训练步骤");
DEFINE_string(test_path, "", "测试集路径，支持文件夹，可选");
DEFINE_string(model_path, "./ftrl.model", "模型输出路径");
DEFINE_string(predict_path, "", "预测结果文件路径，设置为非空则不训练模型，预测测试集数据");
DEFINE_bool(reset_model, false, "不读取已有的模型参数");
DEFINE_string(load_model_from_path, "", "从文件读取已有的模型参数");
DEFINE_uint64(global_id, 0, "配置全局变量id，默认为0即不使用");
DEFINE_int32(iter_times, 1, "训练集迭代次数");
DEFINE_bool(name2id, false, "进行特征id化，在特征为字符串时使用");
DEFINE_string(id2name_model_path, "", "将id映射回特征名称，另外输出模型，配合name2id参数使用");
DEFINE_bool(remove_rare_features, false, "过滤掉出现次数过少的特征");
DEFINE_bool(skip_auc_test, false, "不计算auc");
DEFINE_bool(remove_trivial_features, true, "过滤掉wi=0的特征");
DEFINE_string(strong_features_path, "", "配置强特征id的文件");
DEFINE_bool(fix_strong_features, false, "训练时不改变强特征的权重");
DEFINE_string(test_result_path, "", "测试的auc和ctr结果保存路径");
DEFINE_bool(use_counting_filter, false,
        "使用counting bloom filter控制特征加入，否则使用固定概率随机加入");

int main(int argc, char** argv) {
    if (1 == argc) {
        fprintf(stderr, "use ./ftlr_model --help to show usage\n");
        return -1;
    }

    //load conf
    google::SetVersionString("1.0.0");
    ::google::ParseCommandLineFlags(&argc, &argv, true);

    FtrlModelConfig config;
    if (-1 == loadConfig(FLAGS_conf.c_str(), config)) {
        fprintf(stderr, "Failed to parse ftrl_model config\n");
        return -1;
    }

    if (!LOG_INIT(config.log4config().config_file(),
            config.log4config().category())) {
        fprintf(stderr, "LOG_INIT error [%s, %s]",
                config.log4config().config_file().c_str(),
                config.log4config().category().c_str());
        return -1;
    }

    //init
    LOG_INFO("######Init model######");
    //init sample
    FeatureFilter* feature_filter = NULL;
    if (FLAGS_remove_rare_features) {
        LOG_INFO("Remove_rare_features: true");
        if (FLAGS_use_counting_filter) {
            feature_filter = new CountingBloomFeatureFilter(
                    config.model_params().feature_add_count());
        } else {
            feature_filter = new ProbabilisticFeatureFilter(
                    config.model_params().feature_add_rate());
        }
    }

    String2Id feature_name_to_id_fun = NULL;
    if (FLAGS_name2id) {
        LOG_INFO("Name2id: true");
        feature_name_to_id_fun = &Cityhash;
    }
    SparseBinaryFeatureLabelSample* sample = new SparseBinaryFeatureLabelSample(
            0, feature_filter, feature_name_to_id_fun);

    //init model
    FtrlModel::ModelParams model_params(config.model_params());

    void* shm_addr = ShmAttach(config.model_params().mem_key(),
            config.model_params().mem_size());
    HashShm<uint64_t, FtrlModel::FeatureParams> shm;
    if (NULL == shm_addr) {
        shm_addr = ShmCreate(config.model_params().mem_key(),
                config.model_params().mem_size());
        shm.Init(shm_addr, config.model_params().mem_size());
        shm.Detach();
    }
    if (!shm.Attach(shm_addr, config.model_params().mem_size())) {
        LOG_ERROR("Fail to attach shm");
        return -1;
    }

    FtrlModel model(model_params, &shm, FLAGS_global_id);

    boost::unordered_set<uint64_t> strong_feature_id;
    if (FLAGS_strong_features_path != "") {
        LOG_INFO("Load strong features from %s",
                FLAGS_strong_features_path.c_str());
        if (0
                != load_strong_feature_from_file(FLAGS_strong_features_path,
                        strong_feature_id, feature_name_to_id_fun)) {
            return -1;
        }
        if (FLAGS_fix_strong_features) {
            model.SetFixFeatures(&strong_feature_id);
        }
    }

    //load model
    if (FLAGS_reset_model) { //reset model
        LOG_INFO("Reset model");
        shm.Clear();
        if (FLAGS_global_id > 0 || !strong_feature_id.empty()) {
            model.InitStrongFeatureWeights(strong_feature_id, FLAGS_train_path,
                    sample);
        }
    } else if (FLAGS_load_model_from_path != "") { //load model from file
        LOG_INFO("Load model from %s", FLAGS_load_model_from_path.c_str());
        shm.Clear();
        if (FLAGS_remove_rare_features
                && 0
                        != load_model_from_file_to_filter(
                                FLAGS_load_model_from_path, feature_filter)) {
            return -1;
        }
        if (0 != model.LoadModelFromFile(FLAGS_load_model_from_path)) {
            return -1;
        }
    } else { //use model in mem
        if (FLAGS_remove_rare_features
                && 0 != load_model_from_mem_to_filter(shm, feature_filter)) {
            return -1;
        }
    }

    //only predict
    if (FLAGS_predict_path != "") {
        LOG_INFO("Predict start");
        predict_test(model, sample, FLAGS_test_path, FLAGS_predict_path);
        LOG_INFO("Predict end");
        LOG_INFO("######finish######");
        return 0;
    }

    //train
    if (FLAGS_train_path != "") {
        LOG_INFO("Training start");

        if (feature_filter != NULL) {
            feature_filter->Clear();
        }

        for (int it = 0; it < FLAGS_iter_times; it++) {
            model.Train(FLAGS_train_path, sample);
        }

        if (FLAGS_remove_trivial_features) {
            model.RemoveTrivialFeatures();
        }
        LOG_INFO("Training finish");

        if (0 != model.SaveModelToFile(FLAGS_model_path)) {
            return -1;
        }

        if (FLAGS_id2name_model_path != ""
                && 0
                        != model.SaveModelToFile(FLAGS_id2name_model_path,
                                sample)) {
            return -1;
        }
    }

    //test
    if (!FLAGS_skip_auc_test) {
        LOG_INFO("Testing start");
        string test_file_path;
        if ("" == FLAGS_test_path) {
            test_file_path = FLAGS_train_path;
        } else {
            test_file_path = FLAGS_test_path;
        }

        float auc, avg_ctr, p_ctr;

        if (model.CalAuc(test_file_path, sample, auc, avg_ctr, p_ctr) != 0) {
            return -1;
        }

        LOG_INFO("AUC: %f for test_file (%s)", auc, test_file_path.c_str());
        if (auc < 0.6) {
            LOG_ERROR("AUC is too low");
        }

        if (FLAGS_test_result_path != "") {
            ofstream fout(FLAGS_test_result_path.c_str(),
                    ofstream::out | ofstream::trunc);
            fout << "auc:" << auc << ", avg_ctr:" << avg_ctr << ", p_ctr:"
                    << p_ctr << endl;
            fout.close();
        }
        LOG_INFO("Testing finish");
    }

    LOG_INFO("shm overview: %s", shm.HashHeadToString());

    LOG_INFO("######finish######");

    return 0;
}

//helper
int loadConfig(const char* config_path, FtrlModelConfig& config) {
    if (NULL == config_path) {
        fprintf(stderr, "Invalid config path : %s\n", config_path);
        return -1;
    }

    if (!poseidon::util::ParseProtoFromTextFormatFile(config_path, &config)) {
        fprintf(stderr, "Failed to parse config\n");
        return -1;
    }

    return 0;
}

int load_model_from_file_to_filter(std::string model_path,
        FeatureFilter* feature_filter) {
    using namespace std;
    ifstream fin(model_path.c_str(), ifstream::in);
    if (!fin.good()) {
        LOG_ERROR("Fail to load model from file: %s", model_path.c_str());
        return -1;
    }

    string line;
    while (std::getline(fin, line)) {
        vector<string> vec = split(line, "`");
        if (vec.size() != 4) {
            fin.close();
            LOG_ERROR("Model file's format is incorrect: ", line.c_str());
            return -1;
        }

        uint64_t fid;
        istringstream(vec[0]) >> fid;
        feature_filter->Include(fid);
    }

    fin.close();
    return 0;
}

int load_model_from_mem_to_filter(
        HashShm<uint64_t, FtrlModel::FeatureParams>& shm,
        FeatureFilter* feature_filter) {

    uint64_t* k;
    FtrlModel::FeatureParams* v;
    shm.GotoFirst();
    while (shm.Walk(&k, &v)) {
        feature_filter->Include(*k);
    }
    return 0;
}

int predict_test(Model& model, SparseBinaryFeatureLabelSample* sample,
        string test_path, string predict_path) {

    using namespace std;
    vector<string> files;
    listFiles(test_path, files);
    if (files.empty()) {
        LOG_ERROR("no train files in path: %s", test_path.c_str());
    }

    ofstream fout(predict_path.c_str(), ofstream::out | ofstream::trunc);
    if (!fout.good()) {
        LOG_ERROR("Fail to write predict to file: %s", predict_path.c_str());
        return -1;
    }

    for (vector<string>::iterator it = files.begin(); it != files.end(); it++) {
        LOG_INFO("Stat file: %s", (*it).c_str());
        ifstream fin((*it).c_str());
        if (!fin.good()) {
            LOG_ERROR("Fail to read test file: %s", (*it).c_str());
            return -1;
        }

        while (fin >> *sample) {
            string result = sample->Display();
            fout << result << "`" << model.PredictOne(sample) << endl;
        }

        fin.close();
    }

    fout.close();
    return 0;
}

int load_strong_feature_from_file(string strong_feature_path,
        boost::unordered_set<uint64_t>& strong_feature_set,
        String2Id feature_name_to_id_fun) {
    using namespace std;
    ifstream fin(strong_feature_path.c_str(), ifstream::in);
    if (!fin.good()) {
        LOG_ERROR("Fail to load feature from file: %s",
                strong_feature_path.c_str());
        return -1;
    }

    string line;
    while (std::getline(fin, line)) {
        uint64_t fid;
        if (feature_name_to_id_fun != NULL) {
            fid = feature_name_to_id_fun(line);
        } else {
            istringstream(line) >> fid;
        }
        strong_feature_set.insert(fid);
    }

    fin.close();
    return 0;
}

