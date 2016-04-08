#ifndef TIMER_H
#define TIMER_H
#include <time.h>
#include <queue>
#include <deque>
#include <vector>
#include <algorithm>
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

#include "heap.h"

using namespace std;
struct EventHandler;

typedef void (*TimeOutHandler)(EventHandler* pSession);
struct util_timer
{
public:
    util_timer()
    {
        repeat = false;
        interval = 0;
    }

    void set_handler(TimeOutHandler cb,EventHandler* pSession)
    {
        handler = cb;
        session = pSession;
    }

    time_t expire;

    bool operator < (util_timer *b){
        return this->expire>b->expire;//最小值优先
    }


    bool repeat;
    int interval ;
private:
    TimeOutHandler handler;
    EventHandler* session;

    friend struct timer_compare_little;
    friend struct timer_compare_big;
    friend struct timer_heap;

};

//定义比较结构
struct timer_compare_little{
    bool operator ()(util_timer &a,util_timer &b){
        return a.expire>b.expire;//最小值优先
    }
};

struct timer_compare_big{
    bool operator ()(util_timer &a,util_timer &b){
        return a.expire<b.expire;//最大值优先
    }
};

 int timer_compare(void* data1,void* data2);

struct timer_heap
{
    timer_heap()
    {
        m_pHeap = new Heap(Heap::SmallHeap,timer_compare);
    }

    void add_timer(util_timer* pTimer)
    {
        m_pHeap->Insert(pTimer);
    }

    void delete_timer(util_timer* ptimer)
    {
        if(0 == ptimer)
            return;
    }

    void tick()
    {
        time_t now = time(0);

        if(m_pHeap->Size() == 0)
            return;

        vector<util_timer*> temp;
        while (!m_pHeap->Empty())
        {
            util_timer* pTimer = (util_timer*)m_pHeap->Top();
            if(pTimer->expire > now)
                break ;
            pTimer->handler(pTimer->session);

            m_pHeap->Pop();

            temp.push_back(pTimer);
        }

        for(int i = 0; i < temp.size(); ++i)
        {
            util_timer* pTimer = temp[i];
            if(pTimer->repeat)
            {
                pTimer->expire = time(0) + pTimer->interval;
                m_pHeap->Insert(pTimer);
            }
        }
    }

private:
    Heap* m_pHeap;
};


#endif // TIMER_H
