#include <iostream>
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
#include <set>
#include <map>
#include <string>
#include <list>
#include <deque>

using namespace std;

using namespace std;

#include "xepoll.h"
#include "reactor.h"
#include "reactor.h"
#include "util_timer.h"
#include "sock_comm.h"


void Handler(EventHandler* p)
{
    cout << "Time out" << endl;
}

void Handler2(EventHandler* p)
{
    cout << "xxx"<< endl;
}

void Handler3(EventHandler* p)
{
    cout << "aaaaaa"<< endl;
}



void MultiProcessServer(const char *ip, int port)
{
    int listenFd = Socket(AF_INET,SOCK_STREAM,0);

    sockaddr_in addr;
    memset(&addr,0,sizeof(addr));

    //addr.sin_family = AF_INET;
   // addr.sin_addr= htonl(INADDR_ANY);
    //addr.sin_port = htons(port);

    addr.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&addr.sin_addr);
    addr.sin_port = htons(port);

    Bind(listenFd,(sockaddr*)&addr,sizeof(addr));

    Listen(listenFd,128);

    Signal(SIGCHLD,Default_Sig_Chld);
    Signal(SIGTERM,Default_Sig_Term);
    Signal(SIGPIPE,Default_Sig_Pipe);

    for(;;)
    {

        int clientFd = Accept(listenFd,0,0);

        pid_t pid = 0;

        if((pid = fork()) == 0)
        {
            close(listenFd);
            printf("Welcome!\n");
            sleep(3);
            WriteN(clientFd,"123",4);
            sleep(3);
            WriteN(clientFd,"123",4);
            exit(0);
        }

        close(clientFd);
    }
}

void MultiProcessClient()
{
    int clientFd = Socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in addr;
    socklen_t len = sizeof(addr);

    memset(&addr,0,len);

    addr.sin_family = AF_INET;
    inet_pton(AF_INET,"127.0.0.1",&addr.sin_addr);
    addr.sin_port = htons(88888);

    Connect(clientFd,(sockaddr*)&addr,len);

}

int test(int argc,char** argv)
{
    if(argc < 2)
    {
        printf("Usage: Programe type\n");
        exit(-1);
    }

    for(int i = 0; i < argc; ++i)
    {
        printf("%s\n",argv[i]);
    }
    if(strcmp(argv[1],"0") == 0)
    {
        MultiProcessServer("127.0.0.1",88888);
    }
    else
    {
        MultiProcessClient();
    }
}
#include "log.h"
#include "../OpenSourceCode/log-lib/c-log/src/macro_define.h"
int main()
{
    log_init(LL_DEBUG, "DDDDDDDDD", "./");
    LOG_DEBUG("%s\n","Hello");
    exit(-1);
    ReactorBase* base = lazy_create_reactor("127.0.0.1",8888);
    lazy_set_current_reactor(base);
    lazy_register_timer(base,Handler,0,2,true);
    lazy_register_timer(base,Handler2,0,4,true);
    lazy_register_timer(base,Handler3,0,6,true);
    lazy_loop(base);
    return 0;
}

