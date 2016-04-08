#ifndef REACTOR_H
#define REACTOR_H


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
#include <map>
#include "util_timer.h"
#include "util_timer.h"
#include "reactor.h"

using namespace std;

// 套接字会话
struct EventHandler;

// reactor
struct ReactorBase;

// 数据读取处理
typedef void (*ReadHandler)(EventHandler* pSession);

// 数据写完成处理
typedef void (*WriteHandler)(EventHandler* pSession);

// 错误处理
typedef void (*ErrorHandler)(EventHandler* pSession);

// 会话关闭处理
typedef void (*CloseHandler)(EventHandler* pSession);

// 会话接入处理
typedef void (*AccepteHandler)(EventHandler* pSession);

// 信号处理
typedef void (*SigHandler)(int sig);

// 默认的处理函数
extern void default_accept_handler(EventHandler* pSession);
extern void default_read_handler(EventHandler* pSession);
extern void default_error_handler(EventHandler* pSession);
extern void default_close_handler(EventHandler* pSession);

struct ReactorBase
{
    enum
    {
        max_event_number = 1024,
    };

    ReactorBase()
    {
        epoll_fd = -1;
        listen_fd = -1;
        pipefd[0] = -1;
        pipefd[1] = -1;
        stop_server_flag = false;
        accept_handler = default_accept_handler;
        read_handler = default_read_handler;
        error_handler = default_error_handler;
        close_handler = default_close_handler;
    }

    int epoll_fd;
    int listen_fd;
    int pipefd[2];
    epoll_event events[max_event_number];
    bool stop_server_flag;

    AccepteHandler accept_handler;
    ReadHandler read_handler;
    ErrorHandler error_handler;
    CloseHandler close_handler;

    timer_heap timerheap;

    map<int,SigHandler> sig_handler_map;
};



void addsig(int sig);
EventHandler* lazy_create_listen_fd(const char* ip,unsigned int port);
int lazy_create_epoll_fd();
void lazy_bind_listen_to_epoll(int epoll_fd,EventHandler *pListenSession);
ReactorBase* lazy_create_reactor(const char* ip,unsigned int port);
void lazy_loop(ReactorBase* base);//dispatch/

struct ReactorBase* create_reactor();
struct ReactorBase* lazy_get_current_reactor();
void lazy_set_current_reactor(struct ReactorBase* base);
void lazy_set_accept_handler(ReactorBase* base,AccepteHandler handler);
void lazy_set_read_handler(ReactorBase* base,ReadHandler handler);
void lazy_set_close_handler(ReactorBase* base,CloseHandler handler);
void lazy_set_error_handler(ReactorBase* base,ErrorHandler handler);


void lazy_register_sig(ReactorBase* base,int sig,SigHandler handler);
void lazy_register_timer(ReactorBase* base,TimeOutHandler handler,EventHandler* pSession,int interval,bool repeat);

timer_heap* lazy_get_timer_heap();

#endif // REACTOR_H
