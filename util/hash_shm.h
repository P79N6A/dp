/**
**/

#ifndef _UTIL_HASH_SHM_H_
#define _UTIL_HASH_SHM_H_


//include STD C/C++ head files
#include <sys/shm.h>
#include <string.h>
#include <string>
#include <errno.h>
#include <stdio.h>

//include third_party_lib head files
#include "util/func.h"

namespace poseidon {
namespace util {

template <class KEY, class VALUE>
class HashShm
{
public:
    HashShm();
    virtual ~HashShm();
    bool Attach(void* addr, size_t size);
    void Detach();
    bool Init(void* addr, size_t size);
    void Set(const KEY& key, const VALUE& value);
    bool Get(const KEY& key, VALUE **value);
    bool Has(const KEY& key);
    void Delete(const KEY& key);

    void GotoFirst()
    {
        m_walk_index = 1;
    }
    bool Walk(KEY* key, VALUE* value)
    {
        while (m_walk_index < m_hash_head->empty_offset)
        {
            HashNode* node =  &m_hash_node[m_walk_index++];
            if (node->valid)
            {
                memcpy((char*)key, &node->key, sizeof(KEY));
                memcpy((char*)value, &node->value, sizeof(VALUE));
                return true;
            }
        }
        return false;
    }

    bool Walk(KEY** key, VALUE** value)
    {
        while (m_walk_index < m_hash_head->empty_offset)
        {
            HashNode* node =  &m_hash_node[m_walk_index++];
            if (node->valid)
            {
                *key = &(node->key);
                *value = &(node->value);
                return true;
            }
        }
        return false;
    }


    void Clear();
    const char* GetError()
    {
        return m_error;
    }
public:
    int GetCurrentSize()
    {
        return m_hash_head->current_size;
    }

    int GetCapacity()
    {
        return m_hash_head->capacity;
    }

    const char* HashHeadToString()
    {
        snprintf(m_head_str, sizeof(m_head_str), "capacity = %d, current_size = %d, empty_offset = %d, conflict_count = %d",
            m_hash_head->capacity, m_hash_head->current_size, m_hash_head->empty_offset, m_hash_head->conflict_count);

        return m_head_str;
    }

    int GetFindKeyLinkNum()
    {
        return m_find_key_link_num;
    }

    void SetHashHeadReserve(const std::string& str)
    {
        memcpy(m_hash_head->reserve, str.data(), str.size());
        m_hash_head->reserve_size = str.size();
    }

    bool GetHashHeadReserve(std::string* str)
    {
        if (m_hash_head->reserve_size > 0)
        {
            str->assign(m_hash_head->reserve, m_hash_head->reserve_size);
            return true;
        }
        return false;
    }
protected:
#pragma pack(push, 1)
    struct HashHead
    {
        int capacity; // the total number of elements the hash table can hold
        int current_size; // the number of elements in the hash table
        int empty_offset; // next insert offset
        int conflict_count; // stat set conflict count
        int reserve_size;
        char reserve[1024*1024];
    };

    struct HashNode
    {
        KEY key;
        VALUE value;
        uint32_t hash;
        int link; //offset to the next bucket
        bool valid;
    };
#pragma pack(pop)

    HashHead* m_hash_head;
    HashNode* m_hash_node;

    int* m_hash_index;

    char* m_shm_buff;
    size_t m_shm_size;

    char m_error[256];

    char m_head_str[256];
    int m_find_key_link_num;

    int m_walk_index;
};

template<class KEY, class VALUE>
HashShm<KEY, VALUE>::HashShm()
{
    Detach();
}

template<class KEY, class VALUE>
HashShm<KEY, VALUE>::~HashShm()
{
    Detach();
}

template<class KEY, class VALUE>
void HashShm<KEY, VALUE>::Clear()
{
    memset(m_shm_buff, 0, m_shm_size);

    char* shm_buff = m_shm_buff;

    m_hash_head = (HashHead*)shm_buff;

    m_hash_head->capacity = (m_shm_size - sizeof(HashHead)) / (2 * sizeof(int) + sizeof(HashNode));
    m_hash_head->capacity *= 2;
    m_hash_head->current_size = 0;
    m_hash_head->empty_offset = 1;

    shm_buff += sizeof(HashHead);

    m_hash_index = (int*)shm_buff;

    shm_buff += m_hash_head->capacity * sizeof(int);

    m_hash_node = (HashNode*)shm_buff;
}

template<class KEY, class VALUE>
bool HashShm<KEY, VALUE>::Init(void* addr, size_t size)
{
    m_shm_buff = static_cast<char*>(addr);
    m_shm_size = size;
    char* shm_buff = m_shm_buff;
    m_hash_head = (HashHead*)shm_buff;
    if (m_hash_head->capacity != 0)
    {
        return Attach(addr, size);
    }

    this->Clear();
    return true;
}

template<class KEY, class VALUE>
bool HashShm<KEY, VALUE>::Attach(void* addr, size_t size)
{
    m_shm_buff = static_cast<char*>(addr);
    m_shm_size = size;

    char* shm_buff = m_shm_buff;
    m_hash_head = (HashHead*)shm_buff;
    shm_buff += sizeof(HashHead);        
    m_hash_index = (int*)shm_buff;        
    shm_buff += m_hash_head->capacity * sizeof(int);        
    m_hash_node = (HashNode*)shm_buff;
    
    return true;
}

template<class KEY, class VALUE>
void HashShm<KEY, VALUE>::Detach()
{
    m_shm_buff = NULL;
    m_hash_head = NULL;
    m_hash_node = NULL;
    m_hash_index = NULL;

    m_shm_size = 0;
    m_find_key_link_num = 0;
    m_walk_index = 0;
}   

template<class KEY, class VALUE>
void HashShm<KEY, VALUE>::Set(const KEY& key, const VALUE& value)
{
    uint32_t hash = Func::BytesHash32((char*)(&key),sizeof(KEY));
    int index = hash % m_hash_head->capacity;
    int offset = m_hash_index[index];

    bool is_conflict = false;
    while (offset != 0)
    {
        HashNode* conflict_node =  &m_hash_node[offset];
        if (!conflict_node->valid)
        {
           m_hash_head ->current_size++;
        }
        if (!conflict_node->valid || conflict_node->key == key)
        {
            memcpy(&conflict_node->key, &key, sizeof(KEY));
            memcpy(&conflict_node->value, &value, sizeof(VALUE));
            conflict_node->valid = true;
            return;
        }
        offset = conflict_node->link;
        is_conflict = true;
    }

    if (is_conflict)
    {
        m_hash_head->conflict_count++;
    }
        
    HashNode *new_node = &m_hash_node[m_hash_head->empty_offset];
    new_node->hash = hash;
    memcpy(&new_node->key, &key, sizeof(KEY));
    memcpy(&new_node->value, &value, sizeof(VALUE));
    new_node->link = m_hash_index[index];
    new_node->valid = true;

    m_hash_index[index] = m_hash_head->empty_offset;

    m_hash_head->empty_offset++;
    m_hash_head->current_size++;

}

template<class KEY, class VALUE>
bool HashShm<KEY, VALUE>::Get(const KEY& key, VALUE **value)
{
    uint32_t hash = Func::BytesHash32((char*)(&key),sizeof(KEY));
    int index = hash % m_hash_head->capacity;
    int offset = m_hash_index[index];
    m_find_key_link_num = 0;
    *value = NULL;
    while(offset != 0)
    {
        HashNode* node =  &m_hash_node[offset];
        if (node->key == key && node->valid)
        {
            //memcpy(value, &node->value, sizeof(VALUE));
            *value = &(node->value);
            return true;
        }
        m_find_key_link_num++;
        offset = node->link;
    }

    return false;
}

template<class KEY, class VALUE>
bool HashShm<KEY, VALUE>::Has(const KEY& key)
{
    uint32_t hash = Func::BytesHash32((char*)(&key),sizeof(KEY));
    int index = hash % m_hash_head->capacity;
    int offset = m_hash_index[index];
    m_find_key_link_num = 0;
    while(offset != 0)
    {
        HashNode* node =  &m_hash_node[offset];
        if (node->key == key && node->valid)
        {
            return true;
        }
        m_find_key_link_num++;
        offset = node->link;
    }

    return false;
}

template<class KEY, class VALUE>
void HashShm<KEY, VALUE>::Delete(const KEY& key)
{
    uint32_t hash = Func::BytesHash32((char*)(&key),sizeof(KEY));
    int index = hash % m_hash_head->capacity;
    int offset = m_hash_index[index];
    m_find_key_link_num = 0;
    while(offset != 0)
    {
        HashNode* node =  &m_hash_node[offset];
        if (node->key == key)
        {
            node->valid = false;
            m_hash_head->current_size--;
            return;
        }
        m_find_key_link_num++;
        offset = node->link;
    }
}

} // util
} // poseidon


#endif // _UTIL_HASH_SHM_H_


