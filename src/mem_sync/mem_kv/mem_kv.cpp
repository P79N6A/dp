#include "mem_kv.h"
#include "log.h"

#include "coding.h"
#include "crc.h"
#include "murmur3_hash.h"

#define KGROUP_SIZE 8
#define BUCKET_SIZE 8
#define BLK_META_SIZE 9
#define HEADER_BLK_SIZE 64

#if 0
$ echo 'MEM_KV' |sha1sum |cut -c 1-8
ec1d18a2
#endif
#define MAGIC_NUM "ec1d18a2"
#define PRIMARY_VERSION 1
#define SECONDARY_VERSION 0

namespace poseidon
{
namespace mem_sync
{

enum blk_type_e {
    HEADER_BLK = 1,
    HTB_BLK,
    BUCKET_BLK,
};

enum rc_e {
    RC_ERR = -1,
    RC_OK = 0,
    RC_NOT_FOUND = 1,
    RC_EXIST = 1,
};

static unsigned int hfunc(size_t key_size, const char *key)
{
    return MurmurHash3_x86_32(key, key_size);
}

int wrap_blk_crc(char *blkbuf, size_t blktype, size_t blksize)
{
    uint32_t crc;
    char *p = blkbuf;

    if (blksize > (64 << 20)) {
        LOG_WARN("blksize=%zd too big", blksize);
    }

    p += 4;
    *p++ = blktype & 0xFF;
    p = enc_fix32(p, blksize);

    crc = calc_crc32(blkbuf + 4, blksize - 4);
    enc_fix32(blkbuf, crc);

    return 0;
}

static int check_blk_crc(char *blkbuf, size_t blktype, size_t mxsize, size_t *blksz)
{
    uint32_t crc, crc2, blksize, type;
    char *p = blkbuf;

    crc = dec_fix32(p, &p);
    type = *p++;
    if (type != blktype) {
        LOG_ERROR("blktype=%d not match %d", type, blktype);
        return -1;
    }

    blksize = dec_fix32(p, &p);
    if (blksize > mxsize) {
        LOG_ERROR("blksize=%zd out of bound", blksize);
        return -1;
    }

    if (blksize > (64 << 20)) {
        LOG_WARN("blksize=%zd too big", blksize);
    }

    crc2 = calc_crc32(blkbuf + 4, blksize - 4);

    if (crc != crc2) {
        LOG_ERROR("CRC check fail!!");
        return -2;
    }

    if (blksz != NULL) *blksz = blksize;

    return 0;
}

static int find_in_kgrp(size_t key_size, const char *key, 
                        size_t &val_size, char * &val, char *grpbuf, int kcnt)
{
    int r;
    size_t ksize, vsize, len;
    char *p = grpbuf;

    for (int i = 0; i < kcnt; i++) {
        ksize = dec_varint(p, &p);
        len = ksize > key_size ? key_size : ksize;
        r = memcmp(key, p, len);
        p += ksize;

        if (r < 0 || (r == 0 && key_size < ksize)) return RC_NOT_FOUND;

        if (r == 0 && key_size == ksize) {
            val_size = dec_varint(p, &p);
            val = p;
            return RC_OK;
        } else {
            vsize = dec_varint(p, &p);
            p += vsize;
        }
    }

    return RC_NOT_FOUND;
}

static int locate_kgrp(size_t key_size, const char *key, uint32_t grps[], 
                        int grp_cnt, char *blkbuf)
{
    int r = 0, start, mid, end;
    uint64_t off, ksize, len;
    char *pb, *p;

    pb = blkbuf;
    start = mid = 0;
    end = grp_cnt - 1;

    while (start <= end) {
        mid = start + (end - start) / 2;

        off = grps[mid];
        p = pb + off;

        ksize = *p++;
        len = ksize > key_size ? key_size : ksize;
        
        r = memcmp(key, p, len);

        if (r < 0) {
            end = mid - 1;
        } else if (r > 0) {
            start = mid + 1;
        } else {
            break;
        }
    }

    if (r >= 0) { 
        r = mid; 
    } else {
        r = mid > 0 ? mid - 1 : 0; 
    } 

    return r;
}

static int find_in_bucket(size_t key_size, const char *key, 
                            size_t &val_size, char * &val, char *blkbuf)
{
    int grp_idx = 0, kgrp_cnt, detect, kcnt;
    char *pb, *p;

    p = pb = blkbuf;

    p += BLK_META_SIZE + 4;
    kcnt = dec_fix32(p, &p);
    assert(kcnt > 0);

    kgrp_cnt = (kcnt - 1) / KGROUP_SIZE;
    uint32_t kgrps[kgrp_cnt + 1];
    
    kgrps[0] = (p - pb) + kgrp_cnt * 4;

    for (int i = 1; i <= kgrp_cnt; i++) {
        kgrps[i] = dec_fix32(p, &p);
    }

    detect = kcnt;
    if (kgrp_cnt > 0) {
        grp_idx = locate_kgrp(key_size, key, kgrps, kgrp_cnt + 1, pb);
        assert(grp_idx <= kgrp_cnt);

        if (grp_idx == kgrp_cnt) detect = kcnt - kgrp_cnt * KGROUP_SIZE;
        else detect = KGROUP_SIZE;
    }

    return find_in_kgrp(key_size, key, val_size, val, pb + kgrps[grp_idx], detect);
}

MemKV::MemKV(size_t cap)
{
    _cap = cap;
    _entry_cnt = 0;
    _seri_key_sz = 0;
    _seri_val_sz = 0;
    _seri_entry_sz = 0;
    _valid_bucket_cnt = 0;
    _kvtb = NULL;
    _tb_ptr = NULL;
}

MemKV::~MemKV()
{
    delete [] _kvtb;
}

int MemKV::put(size_t key_size, const char *key, size_t val_size, const char *val)
{
    using namespace std;

    if (_kvtb == NULL) {
        _kvtb = new KVList[_cap];
    }

    size_t pos = hfunc(key_size, key) % _cap;
    map<string, string> &mp = _kvtb[pos];

    pair<map<string, string>::iterator, bool> 
    rp = mp.insert(make_pair<string, string>(string(key, key_size), string(val, val_size)));

    if (rp.second == false) {
        string &old_val = rp.first->second;
        LOG_DEBUG("key %.*s existed, old_val=%s", key_size, key, old_val.c_str());

        _seri_entry_sz = (_seri_entry_sz + val_size) - old_val.size();
        _seri_val_sz += _encode_varint(val_size, NULL);
        _seri_val_sz -= _encode_varint(old_val.size(), NULL);

        old_val.assign(val, val_size);

        return RC_EXIST;
    } else {
        _entry_cnt++;
        _seri_entry_sz += key_size + val_size;
        _seri_key_sz += _encode_varint(key_size, NULL);
        _seri_val_sz += _encode_varint(val_size, NULL);

        if (mp.size() == 1) _valid_bucket_cnt++;

        return RC_OK;
    }
}

int MemKV::get(size_t key_size, const char *key, 
                size_t &val_size, char * &val, char *buf, size_t bsize)
{
    int r, pos;
    char *p, *pb;

    using namespace std;

    pos = hfunc(key_size, key) % _cap;

    if (buf == NULL) {
        assert(_kvtb != NULL);
        map<string, string> &mp = _kvtb[pos];

        map<string, string>::iterator
        it = mp.find(string(key, key_size));

        r = RC_NOT_FOUND;
        if (it != mp.end()) {
            r = RC_OK;
            val_size = it->second.size();
            val = (char *)it->second.data();
        }

        return r;
    }

    r = deseri_hdr_info(buf, bsize);
    if (r != 0) return RC_ERR;

    //locate first item of hashtable
    p = _tb_ptr + pos * BUCKET_SIZE + BLK_META_SIZE;

    //locate bucket block off
    uint64_t bucket_off = dec_fix64(p, NULL);
    if (bucket_off == 0) return RC_NOT_FOUND;

    pb = buf + bucket_off;
    p = pb + BLK_META_SIZE;

    // bucket index matched or not
    int bucket_pos = dec_fix32(p, NULL);
    if (pos != bucket_pos) {
        LOG_ERROR("query pos=%d, real pos=%d", pos, bucket_pos);
        return RC_ERR;
    }

#if 0 // for performance, skip check
    r = check_blk_crc(pb, BUCKET_BLK, bsize, NULL);
    if (r != 0) return RC_ERR;
#endif

    return find_in_bucket(key_size, key, val_size, val, pb);
}

size_t MemKV::calc_seri_size()
{
    uint64_t sz = 0;

    sz += HEADER_BLK_SIZE; //size of header block
    sz += _cap * BUCKET_SIZE + BLK_META_SIZE; //size of hashtable block

    // block meta + bucket_idx(4B) + item_cnt(4B)
    sz += _valid_bucket_cnt * (8 + BLK_META_SIZE); 

    sz += (_entry_cnt / KGROUP_SIZE) * 4; // size of kgroup
    sz += _seri_key_sz; //size of key_size
    sz += _seri_val_sz; //size of val_size

    sz += _seri_entry_sz; //size of kvs

    LOG_INFO("total_size=%d, kvcnt=%d, bucket_cnt=%d", (int)sz, _entry_cnt, _valid_bucket_cnt);
    return sz;
}

int MemKV::serialize(char *buf, size_t bsize)
{
    uint64_t blksz, blktype, kcnt, kgrp_cnt;
    char *ps, *pb, *p, *tp, *tb_ptr, *kgrp_ptr;

    using namespace std;

    //header block
    p = pb = ps = buf;
    
    blktype = HEADER_BLK;
    blksz = HEADER_BLK_SIZE;

    memset(p, 0x00, blksz - (p - pb));
    p += BLK_META_SIZE;
    memcpy(p, MAGIC_NUM, strlen(MAGIC_NUM) > 8 ? 8 : strlen(MAGIC_NUM));
    p += 8;
    *p++ = PRIMARY_VERSION & 0xFF;
    *p++ = SECONDARY_VERSION & 0xFF;

    wrap_blk_crc(pb, blktype, blksz);

    //htb block
    pb += blksz;
    tb_ptr = pb;
    tp = tb_ptr + BLK_META_SIZE; //serialize later
    blksz = _cap * BUCKET_SIZE + BLK_META_SIZE;
    
    //bucket block
    pb += blksz;
    p = pb;
    blktype = BUCKET_BLK;

    for (size_t i = 0; i < _cap; i++) {
        map<string, string> &mp = _kvtb[i];

        if (mp.empty()) {
            memset(tp, 0x00, BUCKET_SIZE);
            tp += BUCKET_SIZE;
            continue;
        }

        
        pb = p; //reset block ptr
        tp = enc_fix64(tp, (p - ps));
        
        p += BLK_META_SIZE;
        p = enc_fix32(p, i);
        
        kcnt = mp.size();
        p = enc_fix32(p, kcnt); 

        kgrp_ptr = p; //serialize later
        kgrp_cnt = (kcnt - 1) / KGROUP_SIZE;

        p += kgrp_cnt * 4;

        //item list
        int j = 0;
        map<string, string>::iterator it;
        for (it = mp.begin(); it != mp.end(); it++, j++) {
            if (j > 0 && (j % KGROUP_SIZE) == 0) {
                kgrp_ptr = enc_fix32(kgrp_ptr, (p - pb));
            }

            const string &key = it->first;
            const string &val = it->second;

            p = enc_varint(p, key.size());
            memcpy(p, key.data(), key.size());
            p += key.size();

            p = enc_varint(p, val.size());
            memcpy(p, val.data(), val.size());
            p += val.size();
        }
        
        wrap_blk_crc(pb, blktype, p - pb);
    }

    wrap_blk_crc(tb_ptr, HTB_BLK, _cap * BUCKET_SIZE + BLK_META_SIZE);

    assert((int)bsize >= (p - ps));
    memset(p, 0x00, (int)bsize - (p - ps));

    delete [] _kvtb;
    _kvtb = NULL;

    return 0;  
}

int MemKV::deseri_hdr_info(char *buf, size_t bsize, bool check_crc)
{
    int r = 0;
    size_t blksz;
    char *p, *pb;
    
    // use the same buf, no need deserialize again
    if (_tb_ptr != NULL && _buf == buf && _bsize == bsize) return 0;

    do {
        _buf = buf;
        _bsize = bsize;

        pb = buf;

        // header block

        if (check_crc) {
            r = check_blk_crc(pb, HEADER_BLK, bsize, &blksz);
            if (r != 0) {
                r = -1;
                break;
            }
        } else { // for performance, skip check
            blksz = dec_fix32(pb + 5, NULL);
        }

        p = pb + BLK_META_SIZE;
        if (memcmp(p, MAGIC_NUM, 8) != 0) {
            r = -2;
            break;
        }

        p += 8;
        if (PRIMARY_VERSION != (int)(*p)) {
            r = -3;
            break;
        }

        // htb block
        pb += blksz;

        if (check_crc) {
            r = check_blk_crc(pb, HTB_BLK, bsize, &blksz);
            if (r != 0) {
                r = -3;
                break;
            }

        } else { // for performance, skip check
            blksz = dec_fix32(pb + 5, NULL);
        }

        _tb_ptr = pb;
        _cap = (blksz - BLK_META_SIZE) / BUCKET_SIZE;
    } while(0);

    if (r < 0) {
        LOG_ERROR("invalid header block, may need upgrade, rc=%d", r);
    }

    return r;
}

MemKVIter *MemKV::get_iter(char *buf, size_t bsize)
{
    MemKVIter *iter = new MemKVIter();
    int r = iter->init(this, buf, bsize);
    if (r != 0) {
        delete iter;
        iter = NULL;
    }

    return iter;
}

MemKVIter::MemKVIter()
{
    _curr_bucket_index = 0;
    _curr_item_index = 0;
    _bucket_blk_off = 0;
}

int MemKVIter::init(MemKV *mtb, char *buf, size_t bsize)
{
    _mtb = mtb;
    int r = _mtb->deseri_hdr_info(buf, bsize, true);
    if (r != 0) return r;

    r = locate_next_bucket(0);
    if (r >= 0) return 0;

    return -1;
}

int MemKVIter::locate_next_bucket(size_t next_bucket_index)
{
    int r;
    char *p;

    if (next_bucket_index >= _mtb->_cap) return 1;

    for (size_t i = next_bucket_index; i < _mtb->_cap; i++) {
        p = _mtb->_tb_ptr + i * BUCKET_SIZE + BLK_META_SIZE;
        _bucket_blk_off  = dec_fix64(p, NULL);
        if (_bucket_blk_off == 0) continue;

        p = _mtb->_buf + _bucket_blk_off;
        r = check_blk_crc(p, BUCKET_BLK, _mtb->_bsize, NULL);
        if (r != 0) return -1;

        p += BLK_META_SIZE + 4;
        _bucket_item_cnt = dec_fix32(p, &p);

        _item_ptr = p + (((_bucket_item_cnt - 1) / KGROUP_SIZE) * 4);
        _curr_bucket_index = i;
        _curr_item_index = 0;

        return 0;
    }

    return 1;
}

int MemKVIter::next(size_t &key_size, char * &key, size_t &val_size, char * &val)
{
    int r;

    if (_curr_item_index >= _bucket_item_cnt) {
        r = locate_next_bucket(_curr_bucket_index + 1);
        if (r != 0) return r;
    }

    char *p = _item_ptr;
    key_size = dec_varint(p, &p);
    key = p;
    p += key_size;
    val_size = dec_varint(p, &p);
    val = p;
    p += val_size;

    _item_ptr = p;
    _curr_item_index++;

    return 0;
}

}
}

 
