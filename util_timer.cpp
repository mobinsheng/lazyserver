#include "util_timer.h"
#include "reactor.h"
#include "reactor.h"

int timer_compare(void *data1, void *data2)
{
    util_timer* t1 = (util_timer*)data1;
    util_timer* t2 = (util_timer*)data2;

    if(t1->expire < t2->expire)
        return -1;
    else if(t1->expire == t2->expire)
        return 0;
    else
        return 1;
}

void lazy_register_timer(ReactorBase* base,TimeOutHandler handler, EventHandler* pSession,int interval, bool repeat)
{
    util_timer* ptimer= new util_timer;
    ptimer->expire = time(0) + interval;
    ptimer->repeat = repeat;
    ptimer->interval = interval;
    ptimer->set_handler(handler,pSession);
    base->timerheap.add_timer(ptimer);
}

timer_heap* lazy_get_timer_heap()
{
    return &(lazy_get_current_reactor()->timerheap);
}
