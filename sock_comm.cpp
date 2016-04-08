#include"sock_comm.h"

// 设置信号处理函数
__sig_handler Signal(int sig, __sig_handler handler)
{
    struct sigaction act,oldact;

    act.sa_handler = handler;

    sigemptyset(&act.sa_mask);

    act.sa_flags = 0;

    if(sig == SIGALRM)
    {
#ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
#endif
    }
    else
    {
#ifdef SA_RESTART
        act.sa_flags |= SA_RESTART;
#endif
    }

    if(sigaction(sig,&act,&oldact) < 0)
        return SIG_ERR;

    return oldact.sa_handler;
}

// 判断本主机的字节顺序
int ByteOrder()
{
    union
    {
        short s;
        char c[sizeof(short)];
    } un;

    un.s = 0x0102;

    if(sizeof(short) == 2)
    {
        if(un.c[0] == 1 && un.c[1] == 2)
            return BIG_ENDIAN;
        else if(un.c[0] == 2 && un.c[1] == 1)
            return LITTLE_ENDIAN;
    }

    return -1;
}

// 创建一个套接字，自带错误处理
int Socket(int domain,int type,int protocol)
{
    int fd =  socket(domain,type,protocol);
    if(fd < 0)
    {
        printf("Create Socket Failed!\n");
        exit(-1);
    }
    return fd;
}

// 连接到服务器，自带错误处理
int Connect(int fd, const sockaddr *addr, socklen_t addrlen)
{
    int ret = connect(fd,addr,addrlen);

    if(ret < 0)
    {
        if(errno == ETIMEDOUT)
        {
            printf("Connect Time Out!\n");
        }
        else if (errno == ECONNREFUSED)
        {
            printf("Server is not exist!\n");
        }
        else if(errno == EHOSTUNREACH || errno == ENETUNREACH)
        {
            printf("Can not reach Server!\n");
        }
        else
        {
            printf("Connect Error!\n");
            exit(-1);
        }
    }

    return ret;
}

// 绑定地址，自带错误处理
int Bind(int fd, const sockaddr *addr, socklen_t addrlen)
{
    int ret = bind(fd,addr,addrlen);

    if(ret < 0)
    {
        if(errno == EADDRINUSE)
        {
            printf("Address had been used!\n");
            exit(-1);
        }
        else
        {
            printf("Bind Error!\n");
            exit(-1);
        }
    }

    return ret;
}

int Listen(int fd, int backlog)
{
    int ret = listen(fd,backlog);

    if(ret < 0)
    {
        printf("Listen Error!\n");
        exit(-1);
    }
    return ret;
}

int Accept(int listenfd, sockaddr *clientAddr, socklen_t* addrlen)
{
    for(;;)
    {
        int clientFd = accept(listenfd,clientAddr,addrlen);

        if(clientFd < 0)
        {
            if(errno == ECONNABORTED)
            {
                Close(clientFd);
                printf("Before Accpet Client had Close!\n");
                continue;
            }

            if(errno == EINTR)
                continue;
            else
            {
                printf("Accpet Error!\n");
                exit(-1);
            }
        }

        return clientFd;
    }

}

int ReadN(int fd, void *buf, int len)
{
    if(len < 0)
        return -1;

    int nLeft = len;
    int nRead = 0;

    char* ptr = (char*)buf;

    while(nLeft > 0)
    {
        if((nRead = read(fd,ptr,nLeft)) < 0)
        {
            if(errno == EINTR)
            {
                nRead = 0;
            }
            else
            {
                return -1;
            }
        }
        else  if(nRead == 0)
        {
                break;
        }

        nLeft -= nRead;
        ptr += nRead;
    }

    return (len - nLeft);
}

int WriteN(int fd, const void *buf, int len)
{
    if(len < 0)
        return -1;

    int nLeft = len;
    int nWrite = 0;

    const char* ptr = (const char*)buf;

    while(nLeft > 0)
    {
        if((nWrite = write(fd,ptr,nLeft)) <=0)
        {
            if(nWrite < 0 && errno == EINTR)
                nWrite = 0;
            else
                return -1;
        }

        nLeft -= nWrite;
        ptr += nWrite;
    }

    return len;
}

int Close(int fd)
{
    return close(fd);
}

sockaddr_in CreateSockaddr(const char *ip, int port)
{
    sockaddr_in addr;

    if(0 == ip || strlen(ip) == 0)
    {
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr= htonl(INADDR_ANY);
        addr.sin_port = htons(port);
    }
    else
    {
        addr.sin_family = AF_INET;
        inet_pton(AF_INET,ip,&addr.sin_addr);
        addr.sin_port = htons(port);
    }

    return addr;
}

int SelectServer(const char* ip,int port)
{
    int client[FD_SETSIZE];

    for(int i = 0; i < FD_SETSIZE; ++i)
    {
        client[i] = -1;
    }

    int listenFd = Socket(AF_INET,SOCK_STREAM,0);

    sockaddr_in addr = CreateSockaddr(ip,port);

    socklen_t addrlen = sizeof(addr);

    Bind(listenFd,(sockaddr*)&addr,addrlen);

    Listen(listenFd,128);

    fd_set read_set;
    fd_set write_set;
    fd_set error_set;

    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_ZERO(&error_set);

    FD_SET(listenFd,&read_set);

    int maxFd = listenFd;

    int maxIndex = 0;

    for(;;)
    {
        fd_set rset = read_set;
        fd_set wset = write_set;
        fd_set eset = error_set;

        int nReady = select(maxFd + 1,&rset,&wset,&eset,NULL);

        if(nReady < 0)
        {
            if(errno == EINTR)
                continue;
            else
            {
                printf("select error!\n");
                exit(-1);
            }
        }
        else if(nReady == 0)
        {
            continue;
        }

        if(FD_ISSET(listenFd,&rset))
        {
            sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);

            int connFd = Accept(listenFd,(sockaddr*)&clientAddr,&clientAddrLen);

            int i = 0;
            for(i = 0; i < FD_SETSIZE; ++i)
            {
                if(client[i] < 0)
                {
                    client[i] = connFd;
                    break;
                }
            }

            if(i >= FD_SETSIZE)
            {
                printf("Too many client!\n");
                Close(connFd);
                continue;
            }

            FD_SET(connFd,&read_set);

            if(maxFd < connFd)
                maxFd = connFd;

            if(i > maxIndex)
                maxIndex = i;

            --nReady;

            if(nReady == 0)
                continue;
        }


        for(int i = 0; i <= maxIndex; ++i)
        {
            int clientFd = client[i];

            if(clientFd < 0)
                continue;

            if(!FD_ISSET(clientFd,&rset))
                continue;

            // do something
            // read

            --nReady;

            if(nReady == 0)
                break;

        }
    }

    return 0;
}

int PollServer(const char *ip, int port)
{
    int listenFd = Socket(AF_INET,SOCK_STREAM,0);

    sockaddr_in addr = CreateSockaddr(ip,port);

    socklen_t addrlen = sizeof(addr);

    Bind(listenFd,(sockaddr*)&addr,addrlen);

    Listen(listenFd,128);

#ifndef OPEN_MAX
#define OPEN_MAX (1024)
#endif

    struct pollfd client[OPEN_MAX];

    //ssize_t
    for(int i = 0; i < OPEN_MAX; ++i)
    {
        client[i].fd = -1;
    }

    client[0].fd = listenFd;
    client[0].events = POLLRDNORM;

    int maxIndex = 0;

    for(;;)
    {
        int nReady = poll(client,maxIndex + 1,INFTIM);

        if(client[0].revents & POLLRDNORM)
        {
            sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);

            int connFd = Accept(listenFd,(sockaddr*)&clientAddr,&clientAddrLen);

            int i = 0;
            for(i = 0; i < OPEN_MAX; ++i)
            {
                if(client[i].fd  < 0)
                {
                    client[i].fd = connFd;
                    client[i].events = POLLRDNORM;
                    break;
                }
            }

            if(i == OPEN_MAX)
            {
                printf("Too many clients!\n");
                Close(connFd);
            }

            if(i > maxIndex)
                maxIndex = i;

            --nReady;
            if(nReady == 0)
                continue;
        }

        for(int i = 0; i < maxIndex + 1; ++i)
        {
            if(client[i].fd < 0)
                continue;

            int clientFd = client[i].fd;

            if(client[i].revents & (POLLRDNORM | POLLERR))
            {
                // handle read
                int nReadBytes = -1;
                if(nReadBytes < 0)
                {
                    if(errno == ECONNRESET)
                    {
                        printf("connecttion reset by client!\n");
                        Close(clientFd);
                        client[i].fd = -1;
                    }
                    else
                    {
                        printf("Read error!\n");
                        exit(-1);
                    }
                }
                else if(nReadBytes == 0)
                {
                    Close(clientFd);
                    client[i].fd = -1;
                }
                // handle error

                --nReady;
                if(nReady == 0)
                    break;
            }
        }

    }
    return 0;
}

void Default_Sig_Chld(int sig)
{
    pid_t pid;

    int stat;

    while((pid = waitpid(-1,&stat,WNOHANG)) > 0)
        printf("SubProcess %d terminated\n",pid);

    return;
}

void Default_Sig_Term(int sig)
{
    printf("Get SIGTERM Sig,process will exit!\n");
    exit(-1);
}

void Default_Sig_Pipe(int sig)
{
    printf("Get SIGPIPE Sig,process will exit!\n");
    exit(-1);
}

