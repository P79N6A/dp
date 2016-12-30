/**
 **/


#include <iostream>

#include "protocol/src/poseidon_ors_model.pb.h"
#include "third_party/cityhash/include/city.h"
#include "util/func.h"

#include "config.h"

using namespace std;
using namespace poseidon;

int loadConfig(const char *config_path, const char* holiday_file, Config &config, fstream &log) {

    if (NULL == config_path) {
        fprintf(stderr, "Invalid config path.");
        return -1;
    }
    config.parse(config_path,holiday_file,log);

    return 0;
}

int main(int argc, char *argv[]) {

    fstream log("run.log", ios::out | ios::trunc | ios::binary);

    if (argc != 5){
        log << "params number less than 4\n";
        log << "params: config_file input_data output_data\n" ;
        return -1;
    }
    char * config_file = argv[1];
    char * holiday_file = argv[2];
    char * input_data = argv[3];
    char * output_data = argv[4];

    Config config;
    loadConfig(config_file,holiday_file, config, log);

    config.print(log);
    log.close();

    poseidon::ors::LRModel lrModel;

    //init meta data
    poseidon::ors::LRModel_Meta* lrModel_meta = lrModel.mutable_meta();
    lrModel_meta->set_model_id((unsigned)config.model_id());
    lrModel_meta->set_negative_down_sampling_rate(config.negative_down_sampling_rate());

    vector<vector<string> > features = config.model_fea_dim();

    log << "init variable\n";

    for (vector<vector<string> >::iterator iterator = features.begin(); iterator != features.end(); ++iterator){
        poseidon::ors::LRModel_CrossFeature *lrModel_crossFeature = lrModel_meta->add_cross_feas();
        log << "meta data: *iterator2 ";
        for (vector<string>::iterator iterator2 = iterator->begin(); iterator2 != iterator->end(); ++iterator2) {
            log << *iterator2 << "\t";
            lrModel_crossFeature->add_feas(*iterator2);
        }
        log << "\n";
    }

    // init transform feature
    vector<vector<string> > fea_transform = config.model_fea_transform();
    for (vector<vector<string> >::iterator iterator = fea_transform.begin(); iterator != fea_transform.end(); ++iterator){
        log << "fea transform data : *iterator\n";
        vector<string> res;
        res.assign(iterator->begin(), iterator->end());
        poseidon::ors::LRModel_TransformFeaVal * lrModel_transformFeaVal = lrModel_meta->add_transform_fea_vals();
        // fea
        // group_fea
        // group_fea val
        // fea val
        lrModel_transformFeaVal->set_raw_fea(res[0]);
        lrModel_transformFeaVal->set_raw_val(res[3]);
        lrModel_transformFeaVal->set_trans_fea(res[1]);
        lrModel_transformFeaVal->set_trans_val(res[2]);
    }

    try{
        fstream input(input_data, ios::in);
        log << "input data name: "<<input_data << "\n";
        string str;
        while( getline(input,str) )
        {
            log << "input line data: "<< str << "\n";
            poseidon::ors::LRModel_FeatureWeight* lrModel_featureWeight = lrModel.add_items();
            vector<string> res;
            boost::char_separator<char> sepa("`");

            if (!GetList(str, res,sepa) || res.size() != 2){
                log << "paser error\n";
                return -1;
            }

            try {
                lrModel_featureWeight->set_fea_hash(CityHash64(res[0].c_str(), res[0].size()));
                lrModel_featureWeight->set_weight(boost::lexical_cast<float >(res[1]));
            }
            catch (const boost::bad_lexical_cast &) {
                log << "error occur: len should be 2" << std::endl;
            }

        }

        fstream output(output_data, ios::out | ios::trunc | ios::binary);

        if (!lrModel.SerializeToOstream(&output)) {
            log << "Failed to write msg.\n" ;
            return -1;
        }

    }catch (exception &e){

    }

    log << "process of data to pb is end....\n";
    log.close();

    return 0;
}
