#include "xepoll.h"
#include "socketsession.h"

epoll::epoll(const char* strIP,unsigned int port,int mode )
{
    m_nEpollFd = epoll_create(5);
    if(m_nEpollFd == -1)
        exit(-1);

    m_nWorkMode = mode;

    bzero(&m_Addr,sizeof(sockaddr_in));

    m_Addr.sin_family = AF_INET;
    inet_pton(AF_INET,strIP,&m_Addr.sin_addr);
    m_Addr.sin_port = htons(port);

    m_strIP = strIP;
    m_nPort = port;

    m_nListenFd = socket(PF_INET,SOCK_STREAM,0);
    if(m_nListenFd < 0)
        exit(-1);

    int nret = bind(m_nListenFd,(sockaddr*)&m_Addr,sizeof(m_Addr));
    if(-1 == nret)
        exit(-1);

    nret = listen(m_nListenFd,5);
    if(-1 == nret)
        exit(-1);

    SocketSession* pSession = new SocketSession;
    pSession->addr = m_Addr;
    pSession->m_nFd = m_nListenFd;

    AddFd(m_nEpollFd,pSession,m_nWorkMode == et_mode,false);

    cout << "Listen Fd = " << m_nListenFd << endl;
}

void epoll::ServerLoop()
{
    while(true)
    {
        int ret = epoll_wait(m_nEpollFd,m_Events,max_event_number,-1);
        if(-1 == ret)
        {
            printf("epoll_wait error!\n");
            exit(-1);
        }
        if(m_nWorkMode == et_mode)
        {
            ET(m_Events,ret,m_nEpollFd,m_nListenFd);
        }
        else
        {
            LT(m_Events,ret,m_nEpollFd,m_nListenFd);
        }

    }
}

// 关闭套接字
void				epoll::Close()
{

}


// 处理客户接入
void    epoll::HandleAccpet(SocketSession* pSession)
{
}

void    epoll::HandleError(SocketSession* pSession)
{
}

// 处理数据可读
void    epoll::HandleRead(SocketSession* pSession)
{

}

// 处理客户离开
void    epoll::HandleClose(SocketSession* pSession)
{
}

int epoll::SetNonBlocking(int fd)
{
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

void epoll::ResetOneShot(int epollfd,SocketSession* pSession)
{
    if(0 == pSession)
        return;

    epoll_event event;
    event.data.ptr = pSession;
    event.events = EPOLLIN | EPOLLET |EPOLLONESHOT;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,pSession->m_nFd,&event);
}


void epoll::AddFd(int epoll_fd,SocketSession* pSession,bool enable_et,bool oneshot )
{
    epoll_event event;
    event.data.ptr = pSession;
    event.events = EPOLLIN;
    if(enable_et)
    {
        event.events |= EPOLLET;

        if(oneshot)
            event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epoll_fd,EPOLL_CTL_ADD,pSession->m_nFd,&event);
    SetNonBlocking(pSession->m_nFd);
}

void epoll::RemoveFd(int epoll_fd,SocketSession* pSession)
{
    epoll_event event;
    event.data.ptr = pSession;
    epoll_ctl(epoll_fd,EPOLL_CTL_DEL,pSession->m_nFd,&event);
}

void epoll::LT(epoll_event* events,int number,int epollfd,int listenfd)
{
    for(int i =0; i < number; ++i)
    {
        SocketSession* pSession = (SocketSession*)events[i].data.ptr;
        int sockfd = pSession->m_nFd;

        if(listenfd == sockfd)
        {
            //HandleAccpet();
            sockaddr_in addr;
            bzero(&addr,sizeof(sockaddr_in));

            socklen_t len = sizeof(addr);
            int connect_fd = accept(listenfd,(sockaddr*)&addr,&len);

            SocketSession* connectSession = new SocketSession;
            connectSession->addr = addr;
            connectSession->m_nFd = connect_fd;

            AddFd(m_nEpollFd,connectSession,m_nWorkMode == et_mode);

            HandleAccpet(connectSession);

            return;
        }

        if(events[i].events & EPOLLIN)
        {
            Buffer* buff = pSession->get_buffer_for_recv();

            if(buff->len == Buffer::max_buffer_size)
            {
                buff = pSession->alloc_recv_buffer();
            }

            int nret = read(sockfd,buff->buffer + buff->len,Buffer::max_buffer_size - buff->len);
            if(nret < 0)
            {
                // error
                printf("read error!\n");
                exit(-1);
            }
            else if(nret == 0)
            {
                HandleClose(pSession);
                RemoveFd(m_nEpollFd,pSession);
                close(pSession->m_nFd);
                delete pSession;
                continue;
            }
            else //if(nret > 0)
            {
                buff->len += nret;
            }
            HandleRead(pSession);
        }

        if(events[i].events & EPOLLOUT)
        {
            list<Buffer*> temp;

            pSession->m_SendBuffer.swap(temp);

            for(list<Buffer*>::iterator it = temp.begin(); it != temp.end(); ++it)
            {
                Buffer* buf = (*it);
                if(buf == 0)
                    continue;
                send(pSession->m_nFd,buf->buffer,buf->len,0);
                pSession->ReleaseBuffer(buf);
            }
        }

        if(events[i].events & EPOLLERR)
        {
            HandleError(pSession);
            RemoveFd(m_nEpollFd,pSession);
            close(pSession->m_nFd);
            delete pSession;
        }
    }
}

void epoll::ET(epoll_event* events,int number,int epollfd,int listenfd)
{
    // 遍历每一个事件
    for(int i =0; i < number; ++i)
    {
        // 获取会话
        SocketSession* pSession = (SocketSession*)events[i].data.ptr;

        if(0 == pSession)
            continue;

        int sockfd = pSession->m_nFd;

        if(listenfd == sockfd)
        {
            sockaddr_in addr;
            bzero(&addr,sizeof(sockaddr_in));

            socklen_t len = sizeof(addr);
            int connect_fd = accept(listenfd,(sockaddr*)&addr,&len);

            SocketSession* connectSession = new SocketSession;
            connectSession->addr = addr;
            connectSession->m_nFd = connect_fd;

            AddFd(m_nEpollFd,connectSession,m_nWorkMode == et_mode);

            HandleAccpet(connectSession);

            return;
        }

        if(events[i].events & EPOLLIN)
        {
            Buffer* buff = pSession->get_buffer_for_recv();

            if(0 == buff)
            {
                printf("alloc Recv Buffer Failed!\n");
                exit(-1);
            }

            bool bReadFinish = false;

            while(true)
            {
                if(buff->len == Buffer::max_buffer_size)
                {
                    buff = pSession->alloc_recv_buffer();
                    if(0 == buff)
                    {
                        printf("alloc Recv Buffer Failed!\n");
                        exit(-1);
                    }
                }

                int nret = read(sockfd,buff->buffer + buff->len,Buffer::max_buffer_size - buff->len);

                if(nret < 0)
                {
                    // 这里表示读取完成
                    if((errno == EAGAIN) || (errno == EWOULDBLOCK))
                    {
                        bReadFinish = true;
                        ResetOneShot(m_nEpollFd,pSession);
                        break;
                    }
                    // 这里表示出错
                    else
                    {
                        // error
                        printf("read error! erron = %d,%s\n",errno,strerror(errno));

                        exit(-1);
                    }
                }
                // 这里表示客户离开
                else if(nret == 0)
                {
                    HandleClose(pSession);
                    RemoveFd(m_nEpollFd,pSession);
                    close(pSession->m_nFd);
                    delete pSession;
                    break;
                }
                // 正确读取到数据
                else //if(nret > 0)
                {
                    buff->len += nret;
                }
            }

            if(bReadFinish)
                HandleRead(pSession);
        }

        // 可以发送数据了
        if(events[i].events & EPOLLOUT)
        {
            list<Buffer*> temp;

            // 把发送缓存的数据取出来
            pSession->m_SendBuffer.swap(temp);

            // 发送每一个buffer
            for(list<Buffer*>::iterator it = temp.begin(); it != temp.end(); ++it)
            {
                Buffer* buf = (*it);
                if(buf == 0)
                    continue;

                // 发送
                send(pSession->m_nFd,buf->buffer,buf->len,0);

                // 释放这些buffer
                pSession->ReleaseBuffer(buf);
            }
        }

        // 表示出错
        if(events[i].events & EPOLLERR)
        {
            HandleError(pSession);
            RemoveFd(m_nEpollFd,pSession);
            close(pSession->m_nFd);
            delete pSession;
        }
    }
}

/////////////////////////////////////////////////////////

