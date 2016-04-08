/*
 * =====================================================================================
 *
 *       Filename:  heap.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016年03月07日 19时12分37秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef _HEAP_H_
#define _HEAP_H_

#include <iostream>
#include <string>
#include <malloc.h>
#include <memory.h>
#include <string.h>
using namespace std;

// 堆数据比较函数
typedef int (*HeapDataCompareFunc)(void* data1,void* data2);

class Heap
{
 public:
    // 堆的类型
    enum
    {
        // 大根堆
        BigHeap,
        // 小根堆
        SmallHeap
    };

    Heap(int type);

    Heap(int type,HeapDataCompareFunc compare);

    ~Heap();

    // 获取堆顶数据
    void* Top();

    // 弹出堆顶数据
    void* Pop();

    // 往堆中插入数据
    void Insert(void* data);

    // 堆的大小
    size_t Size();

    // 是否为空
    bool Empty();

private:

    int m_nType;

    // 插入之后重建堆
    void ReBuild4Insert();

    // 删除一个元素之后重建堆
    void ReBuild4Delete();

    // 判断索引i是否有左孩子
    bool HasLeft(size_t i);
    // 判断索引i是否有右孩子
    bool HasRight(size_t i);

    void** m_Array;
    size_t m_nArrayCapacity;
    size_t m_nArraySize;

    HeapDataCompareFunc m_CompareFunc;
};

#endif
