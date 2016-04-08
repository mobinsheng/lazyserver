#ifndef STL_MEMORY_POOL_H
#define STL_MEMORY_POOL_H

#include <iostream>
using namespace std;

class stl_memory_pool
{
public:
    // 内存分配（分配bytes字节）
    static void* allocate(size_t bytes);
    // 内存回收（回收bytes字节）
    static void deallocate(void* p,size_t bytes);

private:
    // 内存上调的边界
    enum {__ALIGN = 8};
    // 块的最大的大小
    enum {__MAX_BYTES = 128};
    // freelist的个数
    enum {__FREELISTS = __MAX_BYTES / __ALIGN};

    // freelist的节点
    union obj
    {
        union obj* free_list_link;
        char client_data[1];
    };

    // 把bytes上调至8的倍数
    static size_t round_up(size_t bytes)
    {
        return ((bytes + __ALIGN - 1) & ~(__ALIGN - 1));
    }

    // 根据bytes获取freelist的索引（注意bytes应该是经过调整的字节数）
    static size_t freelist_index(size_t bytes)
    {
        return ((bytes + __ALIGN - 1) / __ALIGN  - 1);
    }

    // 构造freelist，如果freelist中的内存块不够了，那么就从空闲堆中取出内存
    // 然后添加到freelist中，如果空闲堆的内存也不够，那么就调用malloc分配内存
    // 然后再分配内存块给freelist
    static void* build_freelist(size_t bytes);
private:
    // freelist数组
    static obj* volatile free_list[__FREELISTS];

    // 空闲堆的起始地址
    static char* start_free;
    // 空闲堆的结束地址
    static char* end_free;
    // 调用malloc从系统取得的总的内存数的大小
    static size_t heap_size;
};

// 一个模板内存池，对stl_memory_pool进行封装
template<class T>
class MemoryPool
{
public:
    static T* Allocate()
    {
        void* p =stl_memory_pool::allocate(sizeof(T));

        T* pT = new(p)T();

        return pT;
    }

    static T* Allocate(const T& t)
    {
        T* p =(T*)stl_memory_pool::allocate(sizeof(T));

        new(p)T(t);

        return p;
    }

    static void DeAllocate(T* p)
    {
        p->~T();
        stl_memory_pool::deallocate(p,sizeof(T));
    }
};

#endif // STL_MEMORY_pool_H
