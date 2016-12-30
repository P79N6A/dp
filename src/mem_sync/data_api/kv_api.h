#ifndef  _MEM_SYNC_KV_API_H_ 
#define  _MEM_SYNC_KV_API_H_

#include <string>

namespace poseidon
{

namespace mem_sync
{

class DataApi;
class MemKV;
class MemKVIter;
class KVIter;

class KVApi
{

public:
    KVApi();

    ~KVApi();

    /**
     * @brief call this before going on other operations.
     * @ret true: success
     *      false: something error
     */
    bool init();

    /**
     * @key Do get via this key.
     * @val Result will copy to it.
     * @ret 0: k-v got success
     *      1: k-v not found
     *     <0: something error
     */
    int get(int dataid, const std::string &key, std::string &val, int *version = NULL);

    /**
     * @key Do get via this key.
     * @val_size Size of value got.
     * @val Pointer to the location of value in share memory. 
     *      Without memory duplication.
     * @ret 0: k-v got success
     *      1: k-v not found
     *     <0: something error
     */
    int get(int dataid, size_t key_size, const char *key, size_t &val_size, 
            char * &val, int *version = NULL);

    /**
     * @brief Get current version of local share memory regarding to @dataid
     * @ret >0: current version got
     *      =0: no local share memory regarding to @dataid, need do memory sync
     *      <0: something error
     */
    int get_version(int dataid);

    /**
     * @brief get iterator for iterating current version of shm regarding to @dataid,
     *        you need delete iter if iteration done
     * @ret NULL if error
     */
    KVIter *get_iter(int dataid);

private:
    size_t   _cap;
    DataApi *_da;
    MemKV   *_kvtb;
};

class KVIter {
    public:
        ~KVIter();

        /**
         * @brief iterate every k-v entry
         * @ret 0: success, k-v entry got
         *      1: EOF, iteration reach end, nothing got
         *     <0: something error
         */
        int next(size_t &key_size, char * &key, size_t &val_size, char * &val);

    private:
        friend class KVApi;

        KVIter();
        int init(MemKV *kvtb, void *shm_ptr, size_t shm_size);

    private:
        MemKVIter *_miter;
};
 
}
}

#endif
