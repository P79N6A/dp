#ifndef _MEM_SYNC_MEM_KV_H_
#define _MEM_SYNC_MEM_KV_H_

#include <string>
#include <map>

namespace poseidon
{
namespace mem_sync
{

class MemKVIter;

class MemKV
{
public:
    friend class MemKVIter;

    MemKV(size_t cap);
    ~MemKV();

    /**
     * @brief  Put k-v into hashtable. 
     * @ret 0: success
     *      1: existed, just replace val
     *     <0: something error
     */
    int put(size_t key_size, const char *key, size_t val_size, const char *val);

    /**
     * @brief  Get val via key. Get in @buf memory if buf not NULL.
     *         You should copy @value if you wana modify it.
     * @ret 0: success
     *      1: not found
     *     <0: something error
     */
    int get(size_t key_size, const char *key, size_t &val_size, 
            char * &val, char *buf = NULL, size_t bsize = 0);

    /**
     * @brief calculate memory size needed for serialization.
     */
    size_t calc_seri_size();

    /**
     * @brief  Serialize to buf.
     */
    int serialize(char *buf, size_t bsize);

    MemKVIter *get_iter(char *buf, size_t bsize);

private:
    int deseri_hdr_info(char *buf, size_t bsize, bool check_crc = false);

private:
    size_t  _cap;            
    size_t  _entry_cnt;
    size_t  _valid_bucket_cnt;
    size_t  _seri_key_sz;
    size_t  _seri_val_sz;
    size_t  _seri_entry_sz;
    size_t  _bsize;

    char   *_tb_ptr;
    char   *_buf;

    typedef std::map<std::string, std::string> KVList;
    KVList *_kvtb;
};

class MemKVIter {
    public:
        MemKVIter();

        int init(MemKV *mtb, char *buf, size_t bsize);

        /**
         * @brief iterate every k-v entry
         * @ret 0: success, k-v entry got
         *      1: EOF, iteration reach end, nothing got
         *     <0: something error
         */
        int next(size_t &key_size, char * &key, size_t &val_size, char * &val);

    private:
        int locate_next_bucket(size_t next_bucket_index);

    private:
        size_t _curr_bucket_index;
        size_t _curr_item_index;
        size_t _bucket_item_cnt;
        size_t _bucket_blk_off;
        char  *_item_ptr;
        MemKV *_mtb;
};


}
}
#endif
