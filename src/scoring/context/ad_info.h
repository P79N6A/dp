#ifndef SRC_SCORING_CONTEXT_AD_INFO_H_
#define SRC_SCORING_CONTEXT_AD_INFO_H_

#include "protocol/src/poseidon_proto.h"
#include "src/scoring/common/common.h"

namespace poseidon {

namespace scoring {

const float kMaxScore = 10;

class AdInfo {
public:
    int Reset(const common::Ad* ad) {
        _ad = ad;
        _score = 1.0;
        _is_exclude = false;
        _is_include = false;
        return 0;
    }
    bool operator <(const AdInfo& other) {
        if (_is_include != other._is_include) {
            return _is_include > other._is_include;
        } else if (_is_exclude != other._is_exclude) {
            return _is_exclude < other._is_exclude;
        } else {
            return _score > other._score;
        }
    }

    float Score() {
        return _score;
    }
    void Score(float score) {
        _score = score;
    }

    float CalPrice(float ctr, float cvr) {
        float org_price = OrgPrice(), price;
        uint32_t bill_type = BillingType();
        switch (bill_type) {
        case CPM:
        case CPT:
        case CPD:
            price = org_price;
            break;
        case CPC:
            price = org_price * ctr * 1000;
            break;
        case CPA:
            price = org_price * ctr * cvr * 1000;
            break;
        default:
            price = 0;
        }
        price /= (1 + PremiumRate());
        LOG_DEBUG("Scoring ad offer price: %d %f", AdId(), price);
        return price;
    }

    void Exclude() {
        _is_exclude = true;
    }
    bool IsExclude() {
        return !_is_include && _is_exclude;
    }

    void Include() {
        _is_include = true;
    }
    bool IsInclude() {
        return _is_include;
    }

    //original ad info
    const common::Ad* Ad() {
        return _ad;
    }
    uint32_t CampaignId() {
        return _ad->campaign_id();
    }
    uint32_t AdId() {
        return _ad->adgroup_id();
    }
    uint64_t CreativeId() {
        return _ad->creative_id();
    }
    float OrgPrice() {
        return _ad->org_price() / 100.0f;
    }
    uint32_t BillingType() {
        return atoi(_ad->billing_type().c_str());
    }
    float PremiumRate() {
        return _ad->premium_rate() / 100.0f;
    }
    bool IsPdb() {
        return _ad->has_pdb_data() && _ad->pdb_data().has_deal_id()
                && _ad->pdb_data().deal_id().size() > 0;
    }
    int32_t ViewType() {
        return _ad->view_type();
    }

private:
    const common::Ad* _ad;
    bool _is_exclude;
    bool _is_include;
    float _score;
};

inline bool AdInfoLess(AdInfo* i, AdInfo* j) {
    return *i < *j;
}

}
}

#endif /* SRC_SCORING_AD_INFO_H_ */
