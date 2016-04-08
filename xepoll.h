#ifndef _XEPOLL_H
#define _XEPOLL_H

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
using namespace std;

struct SocketSession;

class epoll
{
public:
    enum
    {
        max_event_number = 1024,
        et_mode, // ET模式
        lt_mode,  // LT模式
    };

    epoll(const char* strIP,unsigned int port,int mode = et_mode);
    //  主循环
    void ServerLoop();
    // 关闭套接字
    void				Close();

    // 处理客户接入
    virtual void    HandleAccpet(SocketSession* pSession);
    virtual void    HandleError(SocketSession* pSession);
    // 处理数据可读
    virtual void    HandleRead(SocketSession* pSession);
    // 处理客户离开
    virtual void    HandleClose(SocketSession* pSession);
private:
    int SetNonBlocking(int fd);
    void ResetOneShot(int epollfd,SocketSession* pSession);
    void AddFd(int epoll_fd,SocketSession* pSession,bool enable_et,bool oneshot = true);
    void RemoveFd(int epoll_fd,SocketSession* pSession);
    void LT(epoll_event* events,int number,int epollfd,int listenfd);
    void ET(epoll_event* events,int number,int epollfd,int listenfd);
private:
    int m_nEpollFd;
    int m_nListenFd;

    sockaddr_in m_Addr;
    unsigned int m_nPort;
    string m_strIP;

    epoll_event m_Events[max_event_number];

    int m_nWorkMode;
};

#endif // EPOLL_H
