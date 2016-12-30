/**
 **/
#include <ctime>
#include <boost/date_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include "config.h"
#include "util/func.h"

bool GetList(const std::string &src, std::vector<std::string> &res, boost::char_separator<char> sepa) {
    using boost::lexical_cast;
    using boost::bad_lexical_cast;
    bool success = true;
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

    tokenizer tokens(src, sepa);
    for (tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter) {
        try {
            res.push_back(*tok_iter);
        }
        catch (bad_lexical_cast &) {
            success = false;
        }
    }
    return success;
}

bool GetFeatures(const std::string &src, std::vector<std::vector<std::string> > &features_combination) {

    bool success = true;
    std::vector<std::string> res;
    boost::char_separator<char> sepa(",");
    if (!GetList(src, res, sepa)) {
        success = false;
        return success;
    }

    std::vector<std::vector<std::string> > features;
    boost::char_separator<char> sep("^");

    for (std::vector<std::string>::iterator tok_iter = res.begin(); tok_iter != res.end(); ++tok_iter) {
        std::vector<std::string> tmp_features;
        if (!GetList(*tok_iter, tmp_features, sep)) {
            success = false;
            return success;
        }
        features.push_back(tmp_features);
    }

    for (std::vector<std::vector<std::string> >::iterator it_fea = features.begin(); it_fea != features.end();) {

        std::vector<std::string> first_feature;
        first_feature.assign(it_fea->begin(), it_fea->end());

        std::vector<std::string> second_feature;
        if (++it_fea != features.end()) {
            second_feature.assign(it_fea->begin(), it_fea->end());
        }

        std::vector<std::vector<std::string> > tmp_features_combination;

        if (it_fea != features.end()) {

            for (std::vector<std::string>::iterator it_first_feature = first_feature.begin();
                 it_first_feature != first_feature.end(); ++it_first_feature) {
                for (std::vector<std::string>::iterator it_second_feature = second_feature.begin();
                     it_second_feature != second_feature.end(); ++it_second_feature) {

                    if (features_combination.size() == 0) {

                        std::vector<std::string> tmp_v;
                        tmp_v.push_back(*it_first_feature);
                        tmp_v.push_back(*it_second_feature);
                        tmp_features_combination.push_back(tmp_v);

                    } else {

                        for (std::vector<std::vector<std::string> >::iterator it_feature_comb = features_combination.begin();
                             it_feature_comb != features_combination.end(); ++it_feature_comb) {
                            std::vector<std::string> tmp_v;
                            tmp_v.assign(it_feature_comb->begin(), it_feature_comb->end());
                            tmp_v.push_back(*it_first_feature);
                            tmp_v.push_back(*it_second_feature);
                            tmp_features_combination.push_back(tmp_v);

                        }
                    }
                }
            }
        } else {

            if (features_combination.size() == 0) {
                for (std::vector<std::string>::iterator it_first_feature = first_feature.begin();
                     it_first_feature != first_feature.end(); ++it_first_feature) {
                    std::vector<std::string> tmp_v(1, *it_first_feature);
                    tmp_features_combination.push_back(tmp_v);
                }
            } else {

                for (std::vector<std::string>::iterator it_first_feature = first_feature.begin();
                     it_first_feature != first_feature.end(); ++it_first_feature) {
                    for (std::vector<std::vector<std::string> >::iterator it_feature_comb = features_combination.begin();
                         it_feature_comb != features_combination.end(); ++it_feature_comb) {
                        std::vector<std::string> tmp_v;
                        tmp_v.assign(it_feature_comb->begin(), it_feature_comb->end());
                        tmp_v.push_back(*it_first_feature);
                        tmp_features_combination.push_back(tmp_v);

                    }
                }

            }
        }

        features_combination.clear();

        features_combination.assign(tmp_features_combination.begin(), tmp_features_combination.end());

        if (it_fea != features.end()) ++it_fea;

    }

    return success;
}


/**
 * @brief  解析配置文件
 **/
int Config::parse(const std::string &conf_file, const char* holiday_file, std::fstream &log) {
    if (isparse_) {
        return EC_REPARSE;
    }

    boost::property_tree::ini_parser::read_ini(conf_file, pt_);
    conf_file_ = conf_file;
    isparse_ = true;

    model_algo_ = pt_.get<std::string>("model_base_param.model_algo", "").c_str();
    train_type_ = pt_.get<std::string>("model_base_param.train_type", "").c_str();
    fea_dim_cnt_ = atoi(pt_.get<std::string>("model_base_param.fea_dim_cnt", "").c_str());
    negative_down_sampling_rate_ = boost::lexical_cast<float>(
            pt_.get<std::string>("model_base_param.negative_down_sampling_rate", "").c_str());
    log << "init conf\n";

    for (int i = 1; i <= fea_dim_cnt_; i++) {
        std::string index = boost::lexical_cast<std::string>(i);
        std::string fea_dim("model_fea_dim.fea_dim" + index);
        std::string src = pt_.get<std::string>(fea_dim, "").c_str();

        std::vector<std::vector<std::string> > features_combination;
        if (!GetFeatures(src, features_combination)) {
            log << "get features error\n";
        }

        for (std::vector<std::vector<std::string> >::iterator iterator = features_combination.begin();
             iterator != features_combination.end(); ++iterator) {
            for (std::vector<std::string>::iterator iterator1 = iterator->begin();
                 iterator1 != iterator->end(); ++iterator1) {
                features_.insert(*iterator1);
            }
        }

        model_fea_dim_.insert(model_fea_dim_.end(), features_combination.begin(), features_combination.end());
    }

    log << "model_fea_dim_ Got\n";

    get_fea_transform(log);

    get_dt_transform(holiday_file, log);

    return EC_SUCCESS;
}

const char *Config::model_algo() {
    return pt_.get<std::string>("model_base_param.model_algo", "").c_str();
}

const char *Config::train_type() {
    return pt_.get<std::string>("model_base_param.train_type", "").c_str();
}

int Config::fea_dim_cnt() {
    return fea_dim_cnt_;
}

float Config::negative_down_sampling_rate() {
    return negative_down_sampling_rate_;
}

std::vector<std::vector<std::string> > Config::model_fea_dim() {

    return model_fea_dim_;
}

std::set<std::string> Config::features() {
    return features_;
}

int Config::model_id() {
    boost::char_separator<char> sepa("_");
    std::vector<std::string> res;
    if (!GetList(conf_file_, res, sepa)) {
        std::cout << "error occur" << std::endl;
    }

    std::size_t len = res.size();
    if (len != 3) std::cout << "error occur: len should be 3" << std::endl;

    boost::char_separator<char> sepa2(".");
    std::vector<std::string> res2;

    if (!GetList(res[len - 1], res2, sepa2)) {
        std::cout << "error occur" << std::endl;
    }

    std::size_t len2 = res2.size();
    if (len2 != 2) std::cout << "error occur: len should be 2" << std::endl;

    try {
        model_id_ = boost::lexical_cast<int>(res2[0]);
    }
    catch (const boost::bad_lexical_cast &) {
        std::cout << "error occur: len should be 2" << std::endl;
    }

    return model_id_;
}

void Config::get_fea_transform(std::fstream &log) {
    std::string mapping_fea("model_fea_transform.");
    int fea_cnt = boost::lexical_cast<int>(pt_.get<std::string>(mapping_fea + "fea_cnt", ""));

    for (int i = 1; i < fea_cnt + 1; ++i) {
        std::string fea_mapping("fea_mapping.mapping" + boost::lexical_cast<std::string>(i));
        std::string mapping_str(pt_.get<std::string>(fea_mapping, ""));
        std::vector<std::string> mapping;
        boost::char_separator<char> sepa("|");
        GetList(mapping_str, mapping, sepa);

        if (mapping.size() == 2) {
            std::string fea(mapping[0]);
            int mapping_cnt = boost::lexical_cast<int>(pt_.get<std::string>(mapping_fea + fea + "_cnt", ""));
            for (int j = 1; j < mapping_cnt + 1; ++j) {

                std::string fea_mapping_str = pt_.get<std::string>(
                        fea + "_mapping.mapping" + boost::lexical_cast<std::string>(j), "");
                std::vector<std::string> parent_res;
                boost::char_separator<char> sepb("`");
                GetList(fea_mapping_str, parent_res, sepb);

                if (parent_res.size() == 2 && 0 != mapping[0].compare("dt")) {
                    std::vector<std::string> child_res;
                    boost::char_separator<char> sepc("|");
                    if (GetList(parent_res[1], child_res, sepc)) {
                        for (std::vector<std::string>::iterator iterator = child_res.begin();
                             iterator != child_res.end(); ++iterator) {
                            std::vector<std::string> res;
                            // fea
                            res.push_back(mapping[0]);
                            // group_fea
                            res.push_back(mapping[1]);
                            // group_fea val
                            res.push_back(parent_res[0]);
                            // fea val
                            res.push_back(*iterator);
                            model_fea_transform_.push_back(res);
                        }
                    }

                } else if (parent_res.size() == 2 && 0 == mapping[0].compare("dt")) {

                    std::vector<std::string> child_res;
                    boost::char_separator<char> sepc("|");
                    if (GetList(parent_res[1], child_res, sepc)) {
                        for (std::vector<std::string>::iterator iterator = child_res.begin();
                             iterator != child_res.end(); ++iterator) {
                            std::vector<std::string> res;
                            // fea
                            res.push_back(mapping[0]);
                            // group_fea
                            res.push_back(mapping[1]);
                            // group_fea val
                            res.push_back(parent_res[0]);
                            // fea val
                            res.push_back(*iterator);
                            model_dt_transform_.push_back(res);
                        }
                    }

                } else {
                    log << "parsing error: feature transform error\n";
                }
            }
        }
    }
}

void Config::get_dt_transform(const char* path, std::fstream &log) {

    using namespace boost::gregorian;
    days duration(8);
    boost::gregorian::date dt = day_clock::local_day();
    //boost::gregorian::date date1(2016, 10, 1);
    date_period dp(dt, dt + duration);

    std::ifstream holiday_json(path, std::ifstream::binary);

    Json::Reader reader;
    Json::Value root;
    if (reader.parse(holiday_json, root)) {
        log << "json parse successfully!\n";
    } else {
        log << "json parse failed!\n";
        return;
    }

    log << "dt : \n";
    for (boost::gregorian::date dt = dp.begin(); dt != dp.end(); dt = dt + days(1)) {
        std::string df = to_iso_string(dt);
        std::string d = df.substr(6, 2);
        std::string ym = df.substr(0, 6);
        int holiday = -1;

        if (root.isMember(ym) && root[ym].isMember(d)) {

            if ((dt.day_of_week() != Saturday) && (dt.day_of_week() != Sunday)) {
                holiday = 8;
            } else if (root[ym].get(d, "0") == "2") {
                holiday = 8;
            } else {
                holiday = (Sunday == dt.day_of_week() ? 7 : dt.day_of_week() + 0);
            }
        } else {
            holiday = (Sunday == dt.day_of_week() ? 7 : dt.day_of_week() + 0);
        }

        log << df << '\t' << dt.day_of_week() + 0 << '\t' << ym << "\t" << d << "\t" << holiday << "\n";

        for (std::vector<std::vector<std::string> >::iterator iterator = model_dt_transform_.begin();
             iterator != model_dt_transform_.end(); ++iterator) {
            // fea
            // group_fea
            // group_fea val
            // fea val
            try {

                std::vector<std::string> res;
                res.assign(iterator->begin(), iterator->end());
                if (res[0] == "dt" && res[1] == "group_dt" && res[3] == boost::lexical_cast<std::string>(holiday)) {
                    std::vector<std::string> tmp_res;
                    tmp_res.push_back(res[0]);
                    tmp_res.push_back(res[1]);
                    tmp_res.push_back(res[2]);
                    tmp_res.push_back(df);
                    model_fea_transform_.push_back(tmp_res);
                    break;
                }
            }catch (const std::exception &exception){
                std::cerr << exception.what();
            }
        }


    }
    log << "dt end \n";


}

void Config::print(std::fstream &log) {
    log << "model fea transform \n";
    for (std::vector<std::vector<std::string> >::iterator iterator2 = model_fea_transform_.begin();
         iterator2 != model_fea_transform_.end(); ++iterator2) {
        for (std::vector<std::string>::iterator iterator3 = iterator2->begin();
             iterator3 != iterator2->end(); ++iterator3) {
            log << *iterator3 << "\t";
        }
        log << "\n";
    }
    log << "model dt transform \n";
    for (std::vector<std::vector<std::string> >::iterator iterator2 = model_dt_transform_.begin();
         iterator2 != model_dt_transform_.end(); ++iterator2) {
        for (std::vector<std::string>::iterator iterator3 = iterator2->begin();
             iterator3 != iterator2->end(); ++iterator3) {
            log << *iterator3 << "\t";
        }
        log << "\n";
    }
}

std::vector<std::vector<std::string> > Config::model_fea_transform() {
    return model_fea_transform_;
}