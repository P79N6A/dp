/**
 **/

#ifndef  _SCORING_API_H_
#define  _SCORING_API_H_

#include <boost/serialization/singleton.hpp>
#include "protocol/src/poseidon_proto.h"
#include "src/scoring/context/query_context.h"
#include "src/scoring/context/ad_context.h"
#include "src/scoring/scorer/scorer.h"


namespace poseidon
{
namespace scoring
{

class ScoringApi:public boost::serialization::singleton<ScoringApi>
{
public:

    /**
     * @brief               进程启动时调用
     **/
    int Init(const std::string & conf_file);


    /**
     * @brief               每个请求调用一回
     * @param req           [IN]
     * @param rsp           [OUT]
     * @return              成功返回0，否则返回其他错误码
     **/
    int Proc(const ScoringRequest & req, ScoringResponse & rsp);


    /**
     * @brief               进程退出前调用
     **/
    int Fini();

private:
    ScoringConfig _config;
    AdContext _ad_context;
    QueryContext* _qcontext;
    QueryContext _base_query_context;
    Scorer* _scorer;
    Scorer _base_scorer;

    int prepare(const ScoringRequest & req, ScoringResponse& rsp);



};

}
}

#endif   // ----- #ifndef _SCORING_API_H_  -----
