/** 
 * Copyright (c) 2014 Taobao.com
 * All rights reserved.
 * 文件名称：ObjectPool.h
 * 摘要：
 * 作者：jimmy.gj<jimmy.gj@taobao.com>
 * 日期：2015.01.07
 * 修改摘要：
 * 修改者：
 * 修改日期：
 */
#ifndef OBJECTPOOL_H
#define OBJECTPOOL_H

#include <stdint.h>
#include <string.h>
#include <new>
#include <vector>

/**
 * @ingroup utils
 * @brief Template to generate a class that can store objects of another class
 *        either inside a dynamically allocated
 *        memory region or inside a given memory block.
 *
 * It implements a simple reference count system that lets the objects be easily shared.
 * Note that the reference count is manually handled.
 * 对象池通常不用考虑线程安全, 因为总是在同一个线程(通常是主线程)中来分配对象
 */
template <typename T>
class ObjectPool
{
    public:
        /**
         * @brief Creates a new object pool with the given number of objects preallocated either at the given
         *        memory region (the memory region pointer will be incremented by the size needed to store all
         *        data) or if the pointer to the memory pointer is NULL, the template class uses malloc to
         *        allocate memory for his content.
         *
         * @param initNum  The number of objects to hold.
         * @param memory                        Pointer to the memory pointer or NULL if the template should use
         *                                      malloc (new) to allocate memory.
         */
        explicit ObjectPool(size_t initNum = 128, uint8_t** memory = NULL)
        {
            // 事先分配好的内存段
            if (memory)
            {
                //创建object pool
                m_pObjPool = new(static_cast<void *>(*memory)) T[initNum];
                *memory += initNum * sizeof(T);
                //创建引用计数变量
                m_pReferCounts = new( static_cast<void *>( *memory ) ) uint8_t[initNum];
                *memory += initNum;
                // 未动态分配
                m_bDynMemUsed = false;
            }
            // 系统在栈上动态分配(new方法)内存
            else
            {
                m_pObjPool = new T[initNum];
                m_pReferCounts = new uint8_t[initNum];
                // 已动态分配内存
                m_bDynMemUsed = true;
            }

            // 初始化各个成员变量
            if (m_pObjPool && m_pReferCounts)
            {
                memset(m_pReferCounts , 0 , initNum);
                m_nInitNum = initNum;
                m_nObjNums = initNum;
                m_nAvailableObjNums = initNum;
                m_pVecObjPool.push_back(m_pObjPool);
            }
            else
            {
                if (m_bDynMemUsed && m_pObjPool)
                {
                    delete[] m_pObjPool;
                }
                if (m_bDynMemUsed && m_pReferCounts)
                {
                    delete[] m_pReferCounts;
                }
                m_bDynMemUsed = false;
                m_nObjNums = 0;
                m_nAvailableObjNums = 0;
            }
        }

        /**
         * @brief Releases the resources used by this pool in the case the memory was allocated dynamically.
         */
        virtual ~ObjectPool()
        {
            // We have to release memory if it was allocated using new().
            for (unsigned int i = 0; i < m_pVecObjPool.size(); ++i)
            {
                if (!i && !m_bDynMemUsed)
                {
                    continue;
                }
                else
                {
                    // 释放vector中的各个指针指向的数据块
                    if ( m_pVecObjPool[i] )
                    {
                        delete[] m_pVecObjPool[i];
                    }
                }
            }
            if (m_pReferCounts)
            {
                delete[] m_pReferCounts;
            }
        }

        /**
         * @brief Returns the total number of objects that this pool contains.
         *
         * @return Number of objects in pool.
         */
        size_t getObjectCount() const
        {
            return m_nObjNums;
        }

        /**
         * @brief Returns the number of objects actually available.
         *
         * @return Number of available objects in pool.
         */
        size_t getAvailableObjectCount() const
        {
            return m_nAvailableObjNums;
        }

        /**
         * @brief Tries to allocate a new object from the pool and returns a pointer to that object.
         *
         * If no more objects are left inside the pool, the method will alloc more memory.
         *
         * @return Pointer to an object or NULL in the case that alloc memory failed.
         */
        T * allocate()
        {
            if (0 < m_nAvailableObjNums)
            {
                for (unsigned int i = 0 ; i < m_nObjNums ; ++i)
                {
                    if (m_pReferCounts[i] == 0)
                    {
                        m_pReferCounts[i] = 1;
                        m_nAvailableObjNums--;
                        return &m_pVecObjPool[i/m_nInitNum][i%m_nInitNum];
                    }
                }
            }
            else
            {
                m_pObjPool = new T[m_nInitNum];
                uint8_t* pTmp = new uint8_t[m_nObjNums + m_nInitNum];
                if (m_pObjPool && pTmp)
                {
                    memcpy(pTmp, m_pReferCounts, m_nObjNums * sizeof(uint8_t));
                    memset(pTmp + m_nObjNums * sizeof(uint8_t), 0 , m_nInitNum);
                    delete[] m_pReferCounts;
                    m_pReferCounts = pTmp;
                    m_pVecObjPool.push_back(m_pObjPool);
                    m_pReferCounts[m_nObjNums] = 1;
                    m_nAvailableObjNums += m_nInitNum - 1;
                    m_nObjNums += m_nInitNum;
                    return &m_pObjPool[0];
                }
                else
                {
                    if (m_pObjPool)
                    {
                        delete[] m_pObjPool;
                    }
                    if (pTmp)
                    {
                        delete[] pTmp;
                    }
                    return NULL;
                }  
            }
            // default
            return NULL;
        }

        /**
         * @brief Increments the reference count variable for the given object.
         *
         * @param object Pointer to the object for which the reference count should incremented.
         */
        void retain(T* object)
        {
            // 计算索引值
            int index = -1;
            for (unsigned int i = 0; i < m_pVecObjPool.size(); ++i)
            {
                T* pOP = m_pVecObjPool[i];
                if ((size_t)pOP <= (size_t)object <= (size_t)(pOP + m_nInitNum - 1))
                {
                    index = (reinterpret_cast<uint8_t *>( object ) -
                          reinterpret_cast<uint8_t *>(pOP)) / sizeof(T);
                    index += m_nInitNum*i;
                    break;
                }
            }
            // 增加引用计数
            if ( index >= 0 && index < static_cast<int>( m_nObjNums ) )
            {
                m_pReferCounts[index] += 1;
            }
        }

        /**
         * @brief Decrements the reference count variable for the given object and gives it back to the pool if the reference
         *        count reaches 0.
         *
         * @param object Pointer to the object for which the reference count should decremented.
         */
        void release(T* object)
        {
            // 计算索引值
            int index = -1;

            for (unsigned int i = 0; i < m_pVecObjPool.size(); ++i)
            {
                T* pOP = m_pVecObjPool[i];
                if ((size_t)pOP <= (size_t)object <= (size_t)(pOP + m_nInitNum - 1))
                {
                    index = (reinterpret_cast<uint8_t *>( object ) -
                          reinterpret_cast<uint8_t *>(pOP)) / sizeof(T);
                    // 根据分配的对象池大小计算引用计数的索引值
                    index += m_nInitNum*i;
                    break;

                }
            }
            if (index >= 0 && index < static_cast<int>(m_nObjNums))
            {
                // 减小引用计数
                m_pReferCounts[index] -= 1;
                // 如果引用计数为0, 则增加空闲对象数量
                if (!m_pReferCounts[index])
                {
                    m_nAvailableObjNums++;
                }
            }
        }
    private:
        T* m_pObjPool;                // object pool指针, 可以动态变化
        std::vector<T*> m_pVecObjPool; // object pool指针数组
        size_t m_nInitNum;            // 初始化时object的数量,
        uint8_t* m_pReferCounts;      // 引用计数数组指针
        size_t m_nObjNums;            // object总数量
        size_t m_nAvailableObjNums;   // 实际可用的object数量
        bool m_bDynMemUsed;           // 是否为新new出来的内存
};
#endif
