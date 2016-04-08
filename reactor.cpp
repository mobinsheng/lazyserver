#include "reactor.h"
#include "event_handler.h"
#include "reactor.h"
#include "util_timer.h"

static ReactorBase* g_current_reactor = 0;

void            sig_handler                      (int sig);
int               setnonblocking               (int fd);
void            reset_oneshot                 (int epollfd,EventHandler* pSession);
void            removefd_from_epoll     (int epoll_fd,EventHandler* pSession);
void            addfd_to_epoll                (int epollfd,EventHandler* pSession,bool oneshot = true);
void            timer_handler                 (int sig);
extern void            default_sigint_handler   (int sig);
extern void            default_sigpipe_handler(int sig);
extern void            default_sigterm_handler(int sig);
extern void            default_sighup_handler(int sig);
extern void            default_sigchld_handler(int sig);
extern void            default_sigurg_handler(int sig);
extern void            default_sigalrm_handler(int sig);
extern void            default_accept_handler(EventHandler *pSession);
extern void            default_close_handler(EventHandler *pSession);
extern void            default_error_handler(EventHandler *pSession);
extern void            default_read_handler(EventHandler *pSession);

void lazy_set_accept_handler(ReactorBase *base, AccepteHandler handler)
{
    base->accept_handler = handler;
}

void lazy_set_close_handler(ReactorBase *base, CloseHandler handler)
{
    base->close_handler = handler;
}

void lazy_set_error_handler(ReactorBase *base, ErrorHandler handler)
{
    base->error_handler = handler;
}

void lazy_set_read_handler(ReactorBase *base, ReadHandler handler)
{
    base->read_handler = handler;
}

ReactorBase* create_reactor()
{
    ReactorBase* base = new ReactorBase;
    base->epoll_fd = -1;
    base->listen_fd = -1;
    base->pipefd[0] = -1;
    base->pipefd[1] = -1;
    base->stop_server_flag = false;
    return base;
}
ReactorBase* lazy_get_current_reactor()
{
    return g_current_reactor;
}

void lazy_set_current_reactor(ReactorBase *base)
{
    g_current_reactor = base;
}

EventHandler* lazy_create_listen_fd(const char* ip,unsigned int port)
{
    sockaddr_in addr;
    bzero(&addr,sizeof(sockaddr_in));

    addr.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&addr.sin_addr);
    addr.sin_port = htons(port);

    int fd = -1;
    fd = socket(PF_INET,SOCK_STREAM,0);

    if(fd < 0)
        exit(-1);

    int nret = bind(fd,(sockaddr*)&addr,sizeof(addr));
    if(-1 == nret)
        exit(-1);

    nret = listen(fd,5);
    if(-1 == nret)
        exit(-1);

    EventHandler* pSession = new EventHandler;
    pSession->m_Addr = addr;
    pSession->m_nFd = fd;

    return pSession;
}

int lazy_create_epoll_fd()
{
    int epoll_fd = epoll_create(5);
    if(epoll_fd == -1)
        exit(-1);
    return epoll_fd;
}


void lazy_bind_listen_to_epoll(int epoll_fd,EventHandler* pListenSession)
{
    addfd_to_epoll(epoll_fd,pListenSession,false);
}

ReactorBase* lazy_create_reactor(const char* ip,unsigned int port)
{
    EventHandler* pListenSession = lazy_create_listen_fd(ip,port);
    if(pListenSession == 0)
        exit(-1);

    int epoll_fd = epoll_create(5);
    if(epoll_fd == -1)
        exit(-1);

    addfd_to_epoll(epoll_fd,pListenSession,false);

    ReactorBase* base = create_reactor();

    base->epoll_fd = epoll_fd;
    base->listen_fd = pListenSession->m_nFd;
    int ret = socketpair(PF_UNIX,SOCK_STREAM,0,base->pipefd);
    if(ret == -1)
        exit(-1);
    setnonblocking(base->pipefd[1]);
    EventHandler* pSession = new EventHandler;
    pSession->m_nFd = base->pipefd[0];
    addfd_to_epoll(base->epoll_fd,pSession);

    lazy_register_sig(base,SIGHUP,default_sighup_handler);
    lazy_register_sig(base,SIGINT,default_sigint_handler);
    lazy_register_sig(base,SIGTERM,default_sigterm_handler);
    lazy_register_sig(base,SIGCHLD,default_sigchld_handler);
    lazy_register_sig(base,SIGPIPE,default_sigpipe_handler);
    lazy_register_sig(base,SIGURG,default_sigurg_handler);
    lazy_register_sig(base,SIGALRM,default_sigalrm_handler);


    return base;
}

void addfd_to_epoll(int epoll_fd, EventHandler* pSession,bool oneshot)
{
    epoll_event event;
    event.data.ptr = pSession;
    event.events = EPOLLIN;

    event.events |= EPOLLET;

    if(oneshot)
        event.events |= EPOLLONESHOT;

    epoll_ctl(epoll_fd,EPOLL_CTL_ADD,pSession->m_nFd,&event);
    setnonblocking(pSession->m_nFd);
}

int setnonblocking(int fd)
{
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

void sig_handler(int sig)
{
    int save_errno = errno;
    char msg = (char)sig;
    int ret = send(lazy_get_current_reactor()->pipefd[1],(char*)&msg,1,0);
    errno = save_errno;
}

void addsig(int sig)
{
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    sigaction(sig,&sa,0);
}

void lazy_register_sig(ReactorBase* base,int sig, SigHandler handler)
{
    addsig(sig);
    base->sig_handler_map.insert(make_pair(sig,handler));
}

void reset_oneshot(int epollfd,EventHandler* pSession)
{
    if(0 == pSession)
        return;

    epoll_event event;
    event.data.ptr = pSession;
    event.events = EPOLLIN | EPOLLET |EPOLLONESHOT;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,pSession->m_nFd,&event);
}

void removefd_from_epoll(int epoll_fd,EventHandler* pSession)
{
    epoll_event event;
    event.data.ptr = pSession;
    epoll_ctl(epoll_fd,EPOLL_CTL_DEL,pSession->m_nFd,&event);
}



void handle_accpet(ReactorBase* base,EventHandler* pSession)
{
    sockaddr_in addr;
    bzero(&addr,sizeof(sockaddr_in));

    socklen_t len = sizeof(addr);
    int connect_fd = accept(base->listen_fd,(sockaddr*)&addr,&len);

    EventHandler* connectSession = new EventHandler;
    connectSession->m_Addr = addr;
    connectSession->m_nFd = connect_fd;

    addfd_to_epoll(base->epoll_fd,connectSession);

    base->accept_handler(pSession);
}

void handle_sig_timer(ReactorBase* base,EventHandler* pSession)
{
    int sig = -1;
    char signal[1024] = {0};

    int ret = recv(base->pipefd[0],signal,sizeof(signal),0);
    if(ret == -1)
        return;
    else if(ret == 0)
        return;
    else
    {
        for(int i =0; i< ret;++i)
        {
            map<int,SigHandler>::iterator it = lazy_get_current_reactor()->sig_handler_map.find((int)signal[i]);
            if(it != lazy_get_current_reactor()->sig_handler_map.end())
            {
                it->second((int)signal[i]);
            }
            reset_oneshot(base->epoll_fd,pSession);
        }
    }
}

void handle_read(ReactorBase* base,EventHandler* pSession)
{

    bool bReadFinish = false;


    while(true)
    {
        Buffer* buf = pSession->GetLastRecvBuffer();

        if(buf == 0 || buf->size == Buffer::max_buffer_size)
        {
            buf = BufferPool::AllocBuffer();
            pSession->PushRecvBuffer(buf);
        }

        int nret = recv(pSession->m_nFd,buf->buffer + buf->size,Buffer::max_buffer_size - buf->size,0);

        if(nret < 0)
        {
            // 这里表示读取完成
            if((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                bReadFinish = true;
                reset_oneshot(base->epoll_fd,pSession);
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
            base->close_handler(pSession);
            removefd_from_epoll(base->epoll_fd,pSession);
            close(pSession->m_nFd);
            delete pSession;
            return;
        }
        // 正确读取到数据
        else //if(nret > 0)
        {
            buf->size += nret;
        }
    }

    if(bReadFinish)
        default_read_handler(pSession);

    //base->read_handler(pSession);

}


void lazy_loop(ReactorBase* base)
{
    alarm(1);

    while(base->stop_server_flag == false)
    {
        int ret = epoll_wait(base->epoll_fd,base->events,base->max_event_number,-1);
        if(-1 == ret && (errno != EINTR))
        {
            printf("epoll_wait error!\n");
            exit(-1);
        }

        epoll_event* events = base->events;
        int number = ret;
        int epollfd = base->epoll_fd;
        int listenfd = base->listen_fd;

        // 遍历每一个事件
        for(int i =0; i < number; ++i)
        {
            // 获取会话
            EventHandler* pSession = (EventHandler*)events[i].data.ptr;

            if(0 == pSession)
                continue;

            int sockfd = pSession->m_nFd;

            if(listenfd == sockfd)
            {
                handle_accpet(base,pSession);
                continue;
            }
            else if(sockfd == base->pipefd[0] && (events[i].events & EPOLLIN))
            {
                handle_sig_timer(base,pSession);
                continue;
           }
            else if(events[i].events & EPOLLIN)
            {
                handle_read(base,pSession);
            }
            // 可以发送数据了
            else if(events[i].events & EPOLLOUT)
            {
                while(pSession->GetSendBufferCount() != 0)
                {
                    Buffer* buf = pSession->PopSendBuffer();
                    if(buf == 0)
                        continue;
                    send(pSession->m_nFd,buf->buffer,buf->size,0);

                    // 数据发送完成之后要放回内存池子中
                    BufferPool::ReleaseBuffer(buf);
                }
            }
            // 表示出错
            else if(events[i].events & EPOLLERR)
            {
                base->error_handler(pSession);
                removefd_from_epoll(epollfd,pSession);
                close(pSession->m_nFd);
                delete pSession;
            }
        }
    }

    cout << "Server Finish!" << endl;
}


