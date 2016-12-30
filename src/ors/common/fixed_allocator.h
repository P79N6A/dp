/**
**/

#ifndef _ORS_FIXED_ALLOCATOR_H_
#define _ORS_FIXED_ALLOCATOR_H_
//include STD C/C++ head files
#include <vector>
#include <stddef.h>

//include third_party_lib head files


namespace poseidon
{
namespace ors
{
template <typename T>    
class FixedAllocator
{

public:
    FixedAllocator()
    {
        m_current_idx = 0;
    }

    ~FixedAllocator()
    {
    }
    
    void SetQuato(size_t quato)
    {
        m_pool.resize(quato);
        m_current_idx = 0;
    }

    void Reset()
    {
        m_current_idx = 0;
    }

    T* AllocItem()
    {
        if (m_current_idx >= m_pool.size()) {
            return NULL;
        }
        return &m_pool[m_current_idx++];
    }

    int GetAllocNum()
    {
        return m_current_idx;
    }

    T* GetItem(int index)
    {
        if ((size_t)index >= m_current_idx) 
        {
            return NULL;
        }
        return &m_pool[index];
    }

private:
    std::vector<T> m_pool;
    size_t m_current_idx;

};
} // namespace ors
} // namespace poseidon

#endif // _ORS_XXX_H_

