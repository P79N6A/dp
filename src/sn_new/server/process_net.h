/**
 **/

#ifndef  _PROCESS_NET_H_ 
#define  _PROCESS_NET_H_

#include <boost/serialization/singleton.hpp>
#include "comm_event.h"
#include "comm_event_interface.h"
#include "comm_event_factory.h"
#include "protocol/src/poseidon_proto.h"
#include "src/scoring/api/scoring_api.h"
#include "src/sn_new/inverted_index/inverted_index.h"

namespace poseidon{
namespace sn{

//session data
struct SessData;

class ProcessNet:public dc::common::comm_event::CommBase, 
                 public boost::serialization::singleton<ProcessNet> 
{
public:

    int init();

    /**
     * @brief               process req package
     **/
    virtual int handle_read(const char *buf, const int len, struct sockaddr_in &client_addr);

#ifdef FOR_UNIT_TEST
    void TestRequest(SessData *sess);
#endif

protected:
    ProcessNet() {};

private:
    int BuildQuery(SessData *sess, std::vector<Query> &queries);
    int QueryAndFilter(SessData *sess, std::vector<common::Ad> &target_ads);
    int CallScoringApi(SessData *sess, std::vector<common::Ad> &target_ads);
    int Reply(SessData *sess, struct sockaddr_in &client_addr);
    int DoRequest(SessData *sess);

private:
    bool init_;
    std::set<int> inv_targets_;
    scoring::ScoringApi *scoring_api_;
    InvertedIndex inverted_index_;    
};

}//control
}//poseidon

#endif   // ----- #ifndef _PROCESS_ADAPTER_H_  ----- 

