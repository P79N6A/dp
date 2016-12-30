/**
 **/

#ifndef  _PROCESS_NET_H_ 
#define  _PROCESS_NET_H_

#include <boost/serialization/singleton.hpp>
#include "comm_event.h"
#include "comm_event_interface.h"
#include "comm_event_factory.h"
#include "myindex.h"
#include "protocol/src/poseidon_proto.h"

namespace poseidon{
namespace sn{

struct AdInfo
{
    uint64_t adid;          //广告ID
};

struct SessData
{
        uint64_t time_sn_req;
        uint64_t time_sn_query_start;
        uint64_t time_sn_query_end;
        uint64_t time_sn_rsp;
        sn::SNRequest req ;
        sn::SNResponse rsp;
        uint32_t user_city_id;  //user city id
};

class ProcessNet:public dc::common::comm_event::CommBase, public boost::serialization::singleton<ProcessNet> 
{
public:

    enum
    {
        MAX_RETURN_SIZE=64,
    };

    /**
     * @brief               process req package
     **/
    virtual int handle_read(const char * buf, const int len, struct sockaddr_in & client_addr);


    /**
     * @brief               过滤广告
     * @param sess          [IN], Session
     * @param adz           [IN],广告位信息
     * @param ad            [IN],待过滤的广告
     * @return              允许广告返回true, 否则返回false
     **/
    bool filter_ad(const SessData * sess, const sn::AdzInfo & adz, const IndexAd & ad);

    int build_query(const SNRequest & snreq, Query & query, SessData *sess );

    void status_latency(uint64_t sn_latency);


    /**
     * @brief               随机排序
     * @param setad         [IN],广告列表集合
     * @param vr_ad         [OUT],打乱后的列表
     * @return              成功返回0，否则返回其他错误码
     **/
    int rand_sort(const std::set<IndexAd> & setad, std::vector<IndexAd> & vr_ad);

private:

};

}//control
}//poseidon

#endif   // ----- #ifndef _PROCESS_ADAPTER_H_  ----- 

