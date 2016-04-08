#ifndef BUFFER_H
#define BUFFER_H

#include <stdlib.h>
#include <memory.h>
#include <list>
#include <string>
#include <vector>
#include <algorithm>
#include <assert.h>
using namespace std;

class Buffer
{
public:
    enum
    {
        max_buffer_size = 1024,
    };

    Buffer();

    void Clear()
    {
        memset(buffer,0,max_buffer_size);
        size = 0;
    }

    size_t size;
    char buffer[max_buffer_size];
};

struct BufferPool
{
    static Buffer* AllocBuffer()
    {
        if(m_BufferList.size() == 0)
        {
            for(int i = 0; i < 8; ++i)
            {
                Buffer* buf  = new Buffer;
                buf->Clear();
                m_BufferList.push_back(buf);

                m_AllBufferList.push_back(buf);
            }
        }

        Buffer* buf = m_BufferList.front();
        m_BufferList.pop_front();
        return buf;
    }

    static void ReleaseBuffer(Buffer* buf)
    {
        if(buf == 0)
            return;

        buf->Clear();

        m_BufferList.push_back(buf);
    }

    static list<Buffer*> m_BufferList;

    static list<Buffer*> m_AllBufferList;
};

class BufferV
{
 public:
  // 便宜的预先考虑的缓冲区大小
  static const size_t kCheapPrepend = 8;
  // 初始化的缓冲区大小
  static const size_t kInitialSize = 1024;

  BufferV()
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
  void swap(BufferV& rhs)
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
#endif // BUFFER_H
