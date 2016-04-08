#include "reactor.h"
#include "event_handler.h"


void            default_sigint_handler   (int sig)
{
    lazy_get_current_reactor()->stop_server_flag = true;
}

void            default_sigpipe_handler(int sig)
{
    cout << "sig pipe" << endl;
}

void            default_sigterm_handler(int sig)
{
    cout << "sig term" << endl;
}

void            default_sighup_handler(int sig)
{
    cout << "sig hup" << endl;
}

void            default_sigchld_handler(int sig)
{
    cout << "sig chld" << endl;
}

void            default_sigurg_handler(int sig)
{
    cout << "sig urg" << endl;
}

void            default_sigalrm_handler(int sig)
{
    lazy_get_timer_heap()->tick();
    alarm(1);
}


void default_accept_handler(EventHandler *pSession)
{
    cout << "Client Come!" << endl;
}

void default_close_handler(EventHandler *pSession)
{
    cout << "Client Leave!" << endl;
}

void default_error_handler(EventHandler *pSession)
{
    cout << "Client Error!" << endl;
}

void default_read_handler(EventHandler *pSession)
{
    Buffer* buf = pSession->Read();
    if(buf == 0)
        return;

    char sz[2048] = {0};
    memmove(sz,buf->buffer,buf->size);

    BufferPool::ReleaseBuffer(buf);
    cout << sz << endl ;
}
