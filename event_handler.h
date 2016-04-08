#ifndef SOCKETSESSION_H
#define SOCKETSESSION_H


#include <signal.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <algorithm>

#include "buffer.h"

using namespace std;

struct LazyBuffer;


/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size


struct LazyBuffer
{
private:
    LazyBuffer(const LazyBuffer&);
    LazyBuffer& operator = (const LazyBuffer&);
public:
public:
    // 便宜的预先考虑的缓冲区大小
    static const size_t kCheapPrepend = 8;
    // 初始化的缓冲区大小
    static const size_t kInitialSize = 1024;

    LazyBuffer()
        : buffer_(kCheapPrepend + kInitialSize),
          readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend)
    {
        assert(readableBytes() == 0);
        assert(writableBytes() == kInitialSize);
        assert(prependableBytes() == kCheapPrepend);
    }

    // default copy-ctor, dtor and assignment are fine

    // 缓冲区交换
    void swap(LazyBuffer& rhs)
    {
        buffer_.swap(rhs.buffer_);
        std::swap(readerIndex_, rhs.readerIndex_);
        std::swap(writerIndex_, rhs.writerIndex_);
    }

    // 可读的字节的数量
    size_t readableBytes()
    { return writerIndex_ - readerIndex_; }

    // 写空间的剩余字节数
    size_t writableBytes()
    { return buffer_.size() - writerIndex_; }

    // 预先考虑的字节的数量
    // 实际返回的是读索引
    size_t prependableBytes()
    { return readerIndex_; }

    // 取出字符串的其实位置
    const char* peek() const
    { return begin() + readerIndex_; }

    // 查找\r\n
    const char* findCRLF() const
    {
        const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    const char* findCRLF(const char* start) const
    {
        assert(peek() <= start);
        assert(start <= beginWrite());
        const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF+2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    // retrieve returns void, to prevent
    // string str(retrieve(readableBytes()), readableBytes());
    // the evaluation of two functions are unspecified
    // 重新检索，从指定位置开始
    void retrieve(size_t len)
    {
        assert(len <= readableBytes());
        readerIndex_ += len;
    }

    void retrieveUntil(const char* end)
    {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }

    void retrieveAll()
    {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    string retrieveAsString()
    {
        string str(peek(), readableBytes());
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
        return str;
    }

    void append(const string& str)
    {
        append(str.data(), str.length());
    }

    void append(const char* /*restrict*/ data, size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
        assert(len <= writableBytes());
        std::copy(data, data+len, beginWrite());
        writerIndex_ += len;
    }

    void prepend(const void* /*restrict*/ data, size_t len)
    {
        assert(len <= prependableBytes());
        readerIndex_ -= len;
        const char* d = static_cast<const char*>(data);
        std::copy(d, d+len, begin()+readerIndex_);
    }

    void shrink(size_t reserve)
    {
        std::vector<char> buf(kCheapPrepend+readableBytes()+reserve);
        std::copy(peek(), peek()+readableBytes(), buf.begin()+kCheapPrepend);
        buf.swap(buffer_);
    }

    /// Read data directly into buffer.
    ///
    /// It may implement with readv(2)
    /// @return result of read(2), @c errno is saved
    ssize_t readFd(int fd, int* savedErrno);

private:
    char* beginWrite()
    { return begin() + writerIndex_; }

    const char* beginWrite() const
    { return begin() + writerIndex_; }

    char* begin()
    { return &*buffer_.begin(); }

    const char* begin() const
    { return &*buffer_.begin(); }

    void makeSpace(size_t more)
    {
        if (writableBytes() + prependableBytes() < more + kCheapPrepend)
        {
            buffer_.resize(writerIndex_+more);
        }
        else
        {
            // move readable data to the front, make space inside buffer
            size_t used = readableBytes();
            std::copy(begin()+readerIndex_,
                      begin()+writerIndex_,
                      begin()+kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + used;
            assert(used == readableBytes());
        }
    }

private:
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
    static const char kCRLF[];
};

// 套接字会话
struct EventHandler
{
    // 套接字描述符
    int m_nFd;
    // 地址
    sockaddr_in m_Addr;

    EventHandler(){}

    ~EventHandler(){}


    Buffer* Read()
    {
        if(m_RecvBufferList.size() == 0)
            return 0;

        Buffer* buf = m_RecvBufferList.front();
        m_RecvBufferList.pop_front();

        return buf;
    }

    void Write(const char* data,size_t len)
    {
        // 先取出最后发送缓存中最后一个buffer（因为它可能还没有满）
        Buffer* buf =GetLastSendBuffer();
        if(buf ==0 || buf->size == Buffer::max_buffer_size)
        {
            // 分配
            buf = BufferPool::AllocBuffer();
            PushSendBuffer(buf);
        }

        // 如果要发送的数据比较小
        if(len <= (Buffer::max_buffer_size - buf->size))
        {
            memmove(buf->buffer + buf->size, data,len);
            buf->size += len;
            return;
        }
        // 如果要发送的数据很大！
        else
        {
            // 把最后一个buffer填满（最后的这个buffer可能是空的，因为上面有一个AllocBuffer的步骤，但是没有关系）
            memmove(buf->buffer + buf->size,data,Buffer::max_buffer_size - buf->size);

            // 修改数据的偏移
            data += (Buffer::max_buffer_size - buf->size);
            len -= (Buffer::max_buffer_size - buf->size);

            buf->size = Buffer::max_buffer_size;
        }

        // 看看剩余的数据还需要多少个buffer
        int count = (len / Buffer::max_buffer_size) + 1;

        for(int i =0; i < count ; ++i)
        {
            buf = BufferPool::AllocBuffer();
            PushSendBuffer(buf);

            if(i == count - 1)
            {
                // 最后一个buffer
                memmove(buf->buffer,data,len);
                buf->size = len;
            }
            else
            {
                memmove(buf->buffer,data,Buffer::max_buffer_size);
                buf->size = Buffer::max_buffer_size;
                len -= Buffer::max_buffer_size;
                data += Buffer::max_buffer_size;
            }
        }
    }

    Buffer* GetLastRecvBuffer()
    {
        if(m_RecvBufferList.size() == 0)
            return 0;

        Buffer* buf = m_RecvBufferList.back();
        return buf;
    }

    Buffer* GetLastSendBuffer()
    {
        if(m_SendBufferList.size() == 0)
            return 0;

        Buffer* buf = m_SendBufferList.back();
        return buf;
    }

    void PushRecvBuffer(Buffer* buf)
    {
        if(buf == 0)
            return;
        m_RecvBufferList.push_back(buf);
    }

    void PushSendBuffer(Buffer* buf)
    {
        if(buf == 0)
            return ;

        m_SendBufferList.push_back(buf);
    }

    Buffer* PopSendBuffer()
    {
        if(m_SendBufferList.size() == 0)
            return 0;

        Buffer* buf = m_SendBufferList.front();
        m_SendBufferList.pop_front();
        return buf;
    }

    size_t GetSendBufferCount()
    {
        return m_SendBufferList.size();
    }

private:
    EventHandler(const EventHandler&);
    EventHandler& operator = (const EventHandler&);
    list<Buffer*> m_RecvBufferList;
    list<Buffer*> m_SendBufferList;
};


#endif // SOCKETSESSION_H
