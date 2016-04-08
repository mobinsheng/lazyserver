#include "buffer.h"

list<Buffer*> BufferPool::m_BufferList;
list<Buffer*> BufferPool::m_AllBufferList;
Buffer::Buffer()
{
    Clear();
}
