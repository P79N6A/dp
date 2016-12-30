/*
 * cache.h
 * Created on: 2016-12-26
 */

#ifndef SRC_FB_CO_CACHE_H_
#define SRC_FB_CO_CACHE_H_

#include <stdint.h>
#include <sys/time.h>

#include <map>
#include <set>

namespace poseidon {
namespace feedback {

template <typename K, typename V>
class Cache {
public:

    V *Get(const K &key);
    void Set(const K &key, const V &value);
    void OnTimer(int64_t now);

    // for debug
    int CacheSize(void) { return cache_.size(); }
    int CacheKeySize(void) { return timeout_.size(); }

    typedef struct CacheKey {
        bool operator<(const CacheKey &c) const { return time < c.time; }
        K key;
        int64_t time;
    } CacheKey;

private:
    std::map<K, V> cache_;
    std::set<CacheKey> timeout_;
};

template <typename K, typename V>
V *Cache<K, V>::Get(const K &key)
{
    auto it = cache_.find(key);
    if (it == cache_.end())
        return NULL;

    return &(it->second);
}

template <typename K, typename V>
void Cache<K, V>::Set(const K &key, const V &value)
{
    cache_[key] = value;
    CacheKey c;
    c.key = key;

    int64_t time;
    if (timeout_.size()) {
        time = (*timeout_.rbegin()).time;
    } else {
        struct timeval tv;
        ::gettimeofday(&tv, NULL);
        time = (int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec;
    }
    c.time = time;
    timeout_.insert(c);
}

template <typename K, typename V>
void Cache<K, V>::OnTimer(int64_t now)
{
    while (timeout_.size()) {
        auto it = timeout_.begin();
        if (it->time + 1000000 <= now) {
            cache_.erase(it->key);
            timeout_.erase(it);
        } else {
            break;
        }
    }
}

}  // namespace feedback
}  // namespace poseidon

#endif /* SRC_FB_CO_CACHE_H_ */
