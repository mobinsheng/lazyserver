#include "stl_memory_pool.h"
#include <stdlib.h>

// 静态变量的初始化
char* stl_memory_pool::start_free = 0;
char* stl_memory_pool::end_free = 0;
size_t stl_memory_pool::heap_size = 0;

// freelist的初始化
stl_memory_pool::obj* volatile stl_memory_pool::free_list[stl_memory_pool::__FREELISTS] =
{
  0,0,0,0,
  0,0,0,0,
  0,0,0,0,
  0,0,0,0
};

// 分配内存
void* stl_memory_pool::allocate(size_t bytes)
{
    obj* volatile * dest_free_list = 0;
    obj* result = 0;

    // 如果要分配的字节数大于128B，那么直接调用malloc分配
    if(bytes > __MAX_BYTES)
    {
        return malloc(bytes);
    }

    // 字节数上调至8的倍数
    bytes = round_up(bytes);

    // 取得对应的freelist的索引
    size_t index = freelist_index(bytes);

    // 取得对应的freelist
    dest_free_list = free_list+index;

    // 如果对应的freelist已经没有内存块了，那么需要构建freelist
    // 并同时返回内存块给用户
    if((*dest_free_list) == 0)
    {
        return build_freelist(bytes);
    }

    // freelist还有空闲内存块
    result = *dest_free_list;

    *dest_free_list = (*dest_free_list)->free_list_link;

    return result;
}

void stl_memory_pool::deallocate(void *p, size_t bytes)
{
	if (bytes > __MAX_BYTES)
	{
		free(p);
		return;
	}

    bytes = round_up(bytes);
    size_t index = freelist_index(bytes);
    obj* pObj = (obj*)p;
    pObj->free_list_link = 0;
    obj* volatile* dest_free_list = free_list + index;

    if((*dest_free_list) == 0)
    {
        (*dest_free_list) = pObj;
    }
    else
    {
        pObj->free_list_link = (*dest_free_list);
        (*dest_free_list) = pObj;
    }
}

// 注意bytes是经过调整的字节数（上调至8的倍数）
void* stl_memory_pool::build_freelist(size_t bytes)
{
    // 默认一次给freelist分配20个内存块
    size_t nobjs = 20;

    // 内存需求量
    size_t total_size = bytes * nobjs;

    // 空闲堆剩余内存量
    size_t leftbytes = end_free - start_free;

    // 如果空闲的内存量满足需求量
    if(leftbytes >= total_size)
    {
        // 把块分给用户
        obj* result = (obj*)start_free;

        // 剩下的19块分配给freelist
        char* base = start_free + bytes;

        // 取得对应的freelist
        size_t index = freelist_index(bytes);

        obj* volatile * dest_free_list = free_list + index;

        // 把剩下的19个内存块放进freelist中
        for(size_t i = 0; i < nobjs - 1; ++i)
        {
            obj* temp = (obj*)base;
            temp->free_list_link = 0;

            if((*dest_free_list) == 0)
            {
                (*dest_free_list) = temp;
            }
            else
            {
                temp->free_list_link = (*dest_free_list);
                (*dest_free_list) = temp;
            }

            base += bytes;
        }

        // 调整空闲堆的起始位置
        start_free = start_free + total_size;

        return result;
    }
    // 剩余的内存量不能满足需求量，但是可以满足一个以上的内存块
    else if(leftbytes >= bytes)
    {
        // 看看能够满足多少个内存块
        nobjs = (end_free - start_free) / bytes;

        // 调整需求量
        total_size = bytes * nobjs;

        // 把第一块分配给用户
        obj* result = (obj*)start_free;

        // 剩下的nobjs-1块分给freelist
        char* base = start_free + bytes;

        size_t index = freelist_index(bytes);

        obj* volatile * dest_free_list = free_list + index;

        for(size_t i = 0; i < nobjs - 1; ++i)
        {
            obj* temp = (obj*)base;
            temp->free_list_link = 0;

            if((*dest_free_list) == 0)
            {
                (*dest_free_list) = temp;
            }
            else
            {
                temp->free_list_link = (*dest_free_list);
                (*dest_free_list) = temp;
            }

            base += bytes;
        }

        // 调整空闲堆的其实位置
        start_free = start_free + total_size;

        return result;
    }
    // 空闲堆剩余的内存量连一个内存块都满足不了
    else
    {
        // 把剩余的那点内存放进对应的freelist中（不能浪费了`(*∩_∩*)′）
        if(leftbytes > 0)
        {
            size_t index = freelist_index(leftbytes);

            obj* volatile* dest_free_list = free_list + index;

            obj* temp = (obj*)start_free;
            temp->free_list_link = 0;

            if((*dest_free_list) != 0)
            {
                temp->free_list_link = (*dest_free_list);
                (*dest_free_list) = temp;
            }
            else
            {
                (*dest_free_list) = temp;
            }
            start_free = end_free;
        }

        // 此时需要向系统申请内存

        // 向系统申请的内存的大小
        size_t bytes2get = 2 * total_size + round_up(heap_size >> 4);

        start_free = (char*)malloc(bytes2get);

        // 分配失败
        if(0 == start_free)
        {
            // 遍历当前freelist之后的所有freelist，看看是否有满足的内存块，有的话就“借”一块
            size_t index = freelist_index(bytes);

            for(;index < __FREELISTS; ++index)
            {
                obj* result = 0;
                obj* volatile* dest = free_list + index;
                if((*dest) != 0)
                {
                    result = (*dest);
                    (*dest) = (*dest)->free_list_link;
                    return result;
                }
            }

            // 一块都没有，呵呵……，崩溃吧
            exit(-1);
        }

        // 成功申请到内存，调整空闲堆的其实和结束位置
        end_free = start_free + bytes2get;
        heap_size += bytes2get;

        // 递归调用
        return build_freelist(bytes);
    }

    return 0;
}
