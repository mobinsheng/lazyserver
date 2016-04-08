/*
 * =====================================================================================
 *
 *       Filename:  heap.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016年03月07日 19时12分47秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */


#include "heap.h"

int DefaultHeapDataCompareFunc(void* data1,void* data2)
{
    if(data1 < data2)
        return -1;

    if(data1 == data2)
        return 0;

    if(data1 > data2)
        return 1;

}

Heap::Heap(int type)
{
    m_nType = type;
    m_nArraySize = 0;
    m_nArrayCapacity = 2;
    m_Array = new void*[m_nArrayCapacity];
    m_CompareFunc = DefaultHeapDataCompareFunc;
}

Heap::Heap(int type,HeapDataCompareFunc compare)
{
    m_nType = type;
    m_nArraySize = 0;
    m_nArrayCapacity = 2;
    m_Array = new void*[m_nArrayCapacity];
    m_CompareFunc = compare;
}

Heap::~Heap()
{
    if(m_Array != 0)
        delete[] m_Array;

    m_Array = 0;
    m_nArraySize = 0;
}

size_t Heap::Size()
{
    return m_nArraySize;
}

bool Heap::Empty()
{
    if(0 == m_nArraySize)
        return true;

    return false;
}


void Heap::Insert(void *data)
{
    if(m_nArrayCapacity == m_nArraySize)
    {

        void** temp = new void*[m_nArrayCapacity * 2];
        memmove(temp,m_Array,m_nArrayCapacity * sizeof(void*));
        m_nArrayCapacity *= 2;
        delete[] m_Array;
        m_Array = temp;
    }

    m_Array[m_nArraySize++] = data;

    ReBuild4Insert();

}

void* Heap::Top()
{
    if(0 == m_nArraySize)
        return 0;

    return m_Array[0];
}

void* Heap::Pop()
{
    if(m_nArraySize == 0)
        return 0;

    void* data = m_Array[0];

    // 只有一个节点
    if(m_nArraySize == 1)
    {
        --m_nArraySize;
        m_Array[0] = 0;
        return data;
    }

    // 有两个及以上节点

    void* lastData = m_Array[m_nArraySize - 1];
    m_Array[m_nArraySize - 1] = 0;
    m_Array[0] = lastData;

    --m_nArraySize;

    // 重建
    ReBuild4Delete();
    return data;
}



void Heap::ReBuild4Insert()
{
    if(m_nArraySize == 1)
        return;

    size_t i = m_nArraySize - 1;
    size_t halfi = (i - 1) / 2;
    while(i >= 0 && halfi >= 0)
    {
        // 大根堆
        if(m_nType == BigHeap)
        {
            if(m_Array[i] <= m_Array[halfi])
                break;

            void* temp = m_Array[i];
            m_Array[i] = m_Array[halfi];
            m_Array[halfi] = temp;
        }
        // 小根堆
        else
        {
            if(m_Array[i] >= m_Array[halfi])
                break;

            void* temp = m_Array[i];
            m_Array[i] = m_Array[halfi];
            m_Array[halfi] = temp;
        }

        i = halfi;

        if(i <= 0)
            break;

        halfi = (i - 1) / 2;
    }
}

void Heap::ReBuild4Delete()
{
    size_t i = 0;
    size_t doublei = i * 2 + 2;
    while(i < m_nArraySize)
    {
        // 大根堆
        if(m_nType == BigHeap)
        {
            // 有右孩子
            if(HasRight(i))
            {
                if(m_Array[i] >= m_Array[doublei ]
                        && m_Array[i] >= m_Array[doublei - 1])
                    break;

                if(m_Array[doublei] > m_Array[doublei - 1])
                {
                    void* temp = m_Array[i];
                    m_Array[i] = m_Array[doublei ];
                    m_Array[doublei] = temp;

                    i = doublei;
                    doublei = (i + 1) * 2;
                }
                else
                {
                    void* temp = m_Array[i];
                    m_Array[i] = m_Array[doublei - 1];
                    m_Array[doublei - 1] = temp;

                    i = doublei - 1;
                    doublei = (i + 1) * 2;
                }


            }
            // 有左孩子
            else if(HasLeft(i))
            {
                if(m_Array[i] >= m_Array[doublei - 1] )
                    break;

                void* temp = m_Array[i];
                m_Array[i] = m_Array[doublei - 1];
                m_Array[doublei - 1] = temp;

                i = doublei - 1;
                doublei = (i + 1) * 2;
            }
            else
            {
                break;
            }

        }
        //  小根堆
        else
        {
            if(HasRight(i))
            {
                // 如果父亲节点已经比两个子节点还小，那么停止循环
                if(m_Array[i] <= m_Array[doublei ]
                        && m_Array[i] <= m_Array[doublei - 1])
                    break;

                if(m_Array[doublei] < m_Array[doublei - 1])
                {
                    void* temp = m_Array[doublei];
                    m_Array[doublei] = m_Array[i];
                    m_Array[i] = temp;

                    i = doublei ;
                    doublei = (i + 1) * 2;
                }
                else
                {
                    void* temp = m_Array[doublei - 1];
                    m_Array[doublei - 1] = m_Array[i];
                    m_Array[i] = temp;

                    i = doublei - 1;
                    doublei = (i + 1) * 2;
                }
            }
            else if(HasLeft(i))
            {
                if(m_Array[i] <= m_Array[doublei - 1] )
                    break;

                void* temp = m_Array[i];
                m_Array[i] = m_Array[doublei - 1];
                m_Array[doublei - 1] = temp;

                i = doublei - 1;
                doublei = (i + 1) * 2;
            }
            else
            {
                break;
            }
        }
    }
}


bool Heap::HasLeft(size_t i)
{
    if(i * 2 + 1 >= m_nArraySize)
        return false;

    return true;
}

bool Heap::HasRight(size_t i)
{
    if(i * 2 + 2 >= m_nArraySize)
        return false;

    return true;
}
