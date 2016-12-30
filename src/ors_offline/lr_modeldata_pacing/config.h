/**
 **/

#ifndef  _CONFIG_H_ 
#define  _CONFIG_H_

#include <boost/serialization/singleton.hpp>
#include <boost/property_tree/ptree.hpp>    
#include <boost/property_tree/ini_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <string>
#include <vector>
#include <map>
#include <set>

#include <json/json.h>

bool GetList(const std::string &src, std::vector<std::string> &res, boost::char_separator<char> sepa);

class Config:public boost::serialization::singleton<Config>
{
public:
    enum EC
    {
        EC_SUCCESS=0,
        EC_REPARSE,
    };


    Config(): isparse_(false){
    }

    ~Config(){}

    /**
     * @brief  解析配置文件
     **/
    int parse(const std::string & conf_file, const char* holiday_file,  std::fstream &log);

    /**
     * @brief       日志的配置文件
     **/
    const char * model_algo();
    const char * train_type();
    int fea_dim_cnt();
    float negative_down_sampling_rate();
    std::vector<std::vector<std::string> > model_fea_dim();
    std::set<std::string> features();
    int model_id();
    void get_fea_transform(std::fstream & log);
    void get_dt_transform(const char* path, std::fstream & log);

    void print(std::fstream & log);

    std::vector<std::vector<std::string> > model_fea_transform();

private:
    boost::property_tree::ptree pt_; 
    std::string conf_file_;
    int model_id_;
    std::string model_algo_;
    std::string train_type_;
    int fea_dim_cnt_;
    float negative_down_sampling_rate_;
    std::vector<std::vector<std::string> > model_fea_dim_;
    std::set<std::string> features_;
    std::vector<std::vector<std::string> > model_fea_transform_;
    std::vector<std::vector<std::string> > model_dt_transform_;


    bool isparse_;

};

#endif   // ----- #ifndef _CONFIG_H_  ----- 

