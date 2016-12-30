
#ifndef SRC_SCORING_CONTEXT_PARAMS_H_
#define SRC_SCORING_CONTEXT_PARAMS_H_

#include <map>
#include <string>
#include <boost/serialization/singleton.hpp>
#include "protocol/src/poseidon_proto.h"

namespace poseidon {

namespace scoring {


class Params: public boost::serialization::singleton<Params> {
public:
    int Init();
    void UpdateParams();
    AdxParams* GetAdxParams(int32_t source);
    PosParams* GetPosParams(int32_t source, std::string pid);
private:
    std::map<int32_t, AdxParams> _adx_params_map;
    std::map<uint64_t, PosParams> _pos_params_map;
};


}
}

#endif /* SRC_SCORING_CONTEXT_PARAMS_H_ */
