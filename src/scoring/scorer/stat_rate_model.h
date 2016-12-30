#ifndef SRC_SCORING_SCORER_STAT_RATE_MODEL_H_
#define SRC_SCORING_SCORER_STAT_RATE_MODEL_H_

#include "src/scoring/context/query_context.h"
#include "src/scoring/context/ad_context.h"
#include "cxr_model.h"

namespace poseidon {
namespace scoring {

class StatRateModel: public CxrModel {
public:
    StatRateModel();
    ~StatRateModel();
    int Init();
    int Fini();
    int Prepare(QueryContext*);
    float Ctr(AdInfo*);
    float Cvr(AdInfo*);
private:
    uint32_t _source;
    uint32_t _os_type;
    uint64_t _pid;
    uint32_t _advertiser_id;

    float _context_ctr;
    float _context_cvr;
};

}
}

#endif /* SRC_SCORING_SCORER_STAT_RATE_MODEL_H_ */
