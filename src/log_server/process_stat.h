/**
 **/

#ifndef  _PROCESS_STAT_H_ 
#define  _PROCESS_STAT_H_

#include <stdio.h>
#include <iostream>
#include <map>
#include <boost/serialization/singleton.hpp>
#include "hiredis.h"

namespace poseidon
{
namespace log
{


class ProcessStat:public boost::serialization::singleton<ProcessStat>
{
public:

    ProcessStat():redis_context_(NULL)
    {
    }


    /**
     * @brief               redis重新连接
     **/
    int redis_connect();

    /**
     * @brief           初始化
     **/
    int init();
    

    /**
     * @brief           处理单个记录
     **/
    int proc(const char * buf, int buflen);

    int stat_source_pv(const std::string & source);

    int stat_source_ad_pv(const std::string & source, int req_ad_num);

    int stat_deal_req_pv(const std::string & source, const std::string & deal_id, int req_ad_num);

    int stat_source_exp_id_pv(const std::string & source,const std::string & exp_id,int bid_type);

    int stat_source_exp_id_all_pv(const std::string & source,int bid_type);

    int stat_source_exp_id_bid_all_pv(const std::string & source,int bid_type);

    int stat_source_var_exp_id(const std::string & source,const std::string & exp_id);

    int get_cid_and_source(const char * buf, int buflen, std::string &cid, std::string &source);

    int get_value(const char * buf, int buflen, const char * key, std::string & val);

    void report_source(const std::string & source);

    void report_view_type(const std::string & vt);

private:

//    std::map<std::pair<std::string,time_t>, int > sources_st_;    //<source, t> -->count

    std::map<std::string, std::map<time_t, int> > sources_st_;   //source --> std::map<time, count>

    std::map<std::string, std::map<time_t, int> > sources_ad_st_;   //source --> std::map<time, count>

    std::map<std::string, std::map<time_t, int> > sources_exp_st_all;

    std::map<std::string, std::map<time_t, int> > sources_exp_st_bid_all;

    std::map<std::string, std::map<time_t, int> > sources_var_exp_st_all;
    struct DealReqInfo
    {
        std::map<std::string, std::map<time_t, int> > deal_req_pv_;
    };

    std::map<std::string, DealReqInfo> map_req_pv_;//source-->DealReqInfo

    redisContext * redis_context_;  //redis上下文
    std::string redis_host_;        //redis主机
    int redis_port_;                //redis端口

};

}//log
}//poseidon

#endif   // ----- #ifndef _PROCESS_STAT_H_  ----- 

