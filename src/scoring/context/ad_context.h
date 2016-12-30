#ifndef SRC_SCORING_CONTEXT_AD_CONTEXT_H_
#define SRC_SCORING_CONTEXT_AD_CONTEXT_H_

#include <map>
#include <vector>
#include "protocol/src/poseidon_proto.h"
#include "ad_info.h"

namespace poseidon {

namespace scoring {

const size_t kInitAdPoolSize = 20000;
const size_t kMaxAdNum = 80000;

class AdContext {
public:
    int Init();
    int Fini();
    int ResetContext(const ScoringRequest & req);

    size_t AdNum();
    AdInfo* GetAdInfo(uint32_t ad_id);
    //exclude ads and sort ad
    void SortAndFilterAdList();
    //exclude ads and rand ad
    void RandAndFilterAdList();
    std::vector<AdInfo*>* GetAdList();
private:
    std::map<uint32_t, AdInfo*> _ad_map;
    std::vector<AdInfo*> _ad_sort_vec;
    std::vector<AdInfo> _ad_pool;
    size_t _current_idx;

    void reset();
    AdInfo* addAdInfo(uint32_t ad_id);
    AdInfo* getAdInfo(uint32_t ad_id);
};

}
}


#endif /* SRC_SCORING_AD_CONTEXT_H_ */
