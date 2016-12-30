#include <algorithm>

#include "ad_context.h"
#include "util/log.h"

namespace poseidon {

namespace scoring {

int AdContext::Init() {
    _ad_pool.resize(kInitAdPoolSize);
    _ad_sort_vec.resize(kInitAdPoolSize);
    _ad_map.clear();
    return 0;
}

int AdContext::Fini() {
    return 0;
}

int AdContext::ResetContext(const ScoringRequest & req) {
    reset();
    for (int i = 0; i < req.ad_size(); ++i) {
        const common::Ad* ad = &req.ad(i);
        uint32_t ad_id = ad->adgroup_id();
        AdInfo* ad_info = addAdInfo(ad_id);
        if (NULL == ad_info) {
            return -1;
        }
        ad_info->Reset(ad);
    }
    LOG_DEBUG("Scoring request ad num: %d", AdNum());
    return 0;
}

size_t AdContext::AdNum() {
    return _current_idx;
}

AdInfo* AdContext::GetAdInfo(uint32_t ad_id) {
    return _ad_map[ad_id];
}

void AdContext::SortAndFilterAdList() {
    std::sort(_ad_sort_vec.begin(), _ad_sort_vec.end(), AdInfoLess);
    int ad_num = 0;
    while (ad_num < _ad_sort_vec.size()) {
        if (_ad_sort_vec[ad_num]->IsExclude()) {
            break;
        }
        ad_num++;
    }

    _ad_sort_vec.resize(ad_num);
    LOG_DEBUG("Sort and filter scoring ads");
}

void AdContext::RandAndFilterAdList() {
    SortAndFilterAdList();
    srand(time(0));

    std::vector<AdInfo*>::iterator it = _ad_sort_vec.begin();
    while((*it)->IsInclude() && it != _ad_sort_vec.end()) {
        ++it;
    }

    std::random_shuffle(it, _ad_sort_vec.end());
    LOG_DEBUG("Rand and filter scoring ads");
}

std::vector<AdInfo*>* AdContext::GetAdList() {
    return &_ad_sort_vec;
}

void AdContext::reset() {
    _current_idx = 0;
    _ad_map.clear();
    _ad_sort_vec.clear();
}

AdInfo* AdContext::addAdInfo(uint32_t adgroup_id) {
    AdInfo* r_ptr = _ad_map[adgroup_id];
    if (r_ptr != NULL) {
        return r_ptr;
    }
    if (_current_idx >= _ad_pool.size() && _current_idx < kMaxAdNum) {
        _ad_pool.resize(_ad_pool.size() * 2);
    } else if (_current_idx >= kMaxAdNum) {
        LOG_ERROR("Ad num larger than %d", kMaxAdNum);
        return NULL;
    }
    r_ptr = &_ad_pool[_current_idx++];
    _ad_sort_vec.push_back(r_ptr);
    _ad_map[adgroup_id] = r_ptr;
    return r_ptr;
}

AdInfo* AdContext::getAdInfo(uint32_t ad_id) {
    return _ad_map[ad_id];
}

}
}
