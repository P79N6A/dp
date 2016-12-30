
#ifndef SRC_SCORING_SCORER_CXR_MODEL_H_
#define SRC_SCORING_SCORER_CXR_MODEL_H_

#include "src/scoring/context/query_context.h"
#include "src/scoring/context/ad_context.h"

namespace poseidon {
namespace scoring {


class CxrModel {
public:
    CxrModel() {}
    virtual ~CxrModel() {};
    virtual int Init() = 0;
    virtual int Fini() = 0;
    virtual int Prepare(QueryContext*) = 0;
    virtual float Ctr(AdInfo*) = 0;
    virtual float Cvr(AdInfo*) = 0;
};

}
}


#endif /* SRC_SCORING_SCORER_CXR_MODEL_H_ */
