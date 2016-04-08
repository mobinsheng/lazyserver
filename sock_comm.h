#ifndef SOCK_COMM_H
#define SOCK_COMM_H

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#ifdef HAVE_MQUEUE_H
# include <mqueue.h>
#endif
#ifdef HAVE_SEMAPHORE_H
# include <semaphore.h>
#ifndef SEM_FAILED
#define SEM_FAILED ((sem_t *)(-1))
#endif
#endif
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#ifndef MAP_FAILED
#define MAP_FAILED ((void *)(-1))
#endif
#ifdef HAVE_SYS_IPC_H
# include <sys/ipc.h>
#endif
#ifdef HAVE_SYS_MSG_H
# include <sys/msg.h>
#endif
#ifdef HAVE_SYS_SEM_H
#ifdef __bsdi__
#undef HAVE_SYS_SEM_H
#else
# include <sys/sem.h> /* System V semaphores */
#endif
#ifndef HAVE_SEMUN_UNION
/* $$.It semun$$ */
union semun { /* define union for semctl() */
  int              val;
  struct semid_ds *buf;
  unsigned short  *array;
};
#endif
#endif /* HAVE_SYS_SEM_H */
#ifdef HAVE_SYS_SHM_H
# include <sys/shm.h> /* System V shared memory */
#endif
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h> /* for convenience */
#endif
#ifdef HAVE_POLL_H
# include <poll.h> /* for convenience */
#endif
#ifdef HAVE_STROPTS_H
# include <stropts.h> /* for convenience */
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h> /* for convenience */
#endif
/* Next three headers are normally needed for socket/file ioctl's:
 * <sys/ioctl.h>, <sys/filio.h>, and <sys/sockio.h>.
 */
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#endif
#ifdef HAVE_DOOR_H
# include <door.h> /* Solaris doors API */
#endif
#ifdef HAVE_RPC_RPC_H
#ifdef _PSX4_NSPACE_H_TS /* Digital Unix 4.0b hack, hack, hack */
#undef SUCCESS
#endif
# include <rpc/rpc.h> /* Sun RPC */
#endif
/* Define bzero() as a macro if it's not in standard C library. */
#ifndef HAVE_BZERO
#define bzero(ptr,n) memset(ptr, 0, n)
#endif
/* Posix.1g requires that an #include of <poll.h> DefinE INFTIM, but many
   systems still DefinE it in <sys/stropts.h>.  We don't want to include
   all the streams stuff if it's not needed, so we just DefinE INFTIM here.
   This is the standard value, but there's no guarantee it is -1. */
#ifndef INFTIM
#define INFTIM          (-1)    /* infinite poll timeout */
#ifdef HAVE_POLL_H
#define INFTIM_UNPH /* tell unpxti.h we defined it */
#endif
#endif


#include <string.h>
#include <time.h>
#include <stdio.h>

#include <stdarg.h>

#include <iostream>
#include <fstream>
#include <map>
#include <deque>
#include <vector>
#include <set>
#include <list>
#include <string>
#include <ctime>
#include <fstream>
#include <sstream>
#include <exception>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>
#if (defined(WIN32) || defined(WIN64))
#include <direct.h>
#include <process.h>
#include <tchar.h>
#include <WINSOCK2.H>
#include <Mswsock.h>
#include <windows.h>
#ifdef _DEBUG
#include <assert.h>
#endif

#include <io.h>
#include <winsock.h>
#include <wininet.h>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#pragma comment(lib,"WS2_32")
#pragma comment(lib,"odbc32")

#else
#include<pthread.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <sys/types.h>		/* some systems still require this */
#include <sys/stat.h>
#include <sys/termios.h>	/* for winsize */

#ifndef TIOCGWINSZ
#include <sys/ioctl.h>
#endif

#include <stdio.h>		/* for convenience */
#include <stdlib.h>		/* for convenience */
#include <stddef.h>		/* for offsetof */
#include <string.h>		/* for convenience */
#include <unistd.h>		/* for convenience */
#include <signal.h>		/* for SIG_ERR */
#include <poll.h>
#endif

using namespace std;


typedef void (*__sig_handler)(int sig);
typedef void (*__accpet_handler)(int connFd,sockaddr_in& addr);
typedef void (*__read_handler)(int connFd,char* buf,int len);
typedef void (*__write_handler)(int connFd,char* buf,int len);
typedef void (*__error_handler)(int connFd);

__sig_handler Signal(int sig,void (*__sig_handler)(int));//sig_handler handler

#ifndef BIG_ORDER
#define BIG_ORDER (1)
#endif

#ifndef LITTLE_ORDER
#define LITTLE_ORDER (0)
#endif

// 判断字节顺序
int ByteOrder();

// 创建套接字（已经处理了错误的情况）
int Socket(int domain,int type,int protocol);

// 连接服务器（已经处理了错误的情况）
int Connect(int fd,const struct sockaddr* addr,socklen_t addrlen);

// 绑定地址（处理了错误的情况）
int Bind(int fd,const struct sockaddr* addr,socklen_t addrlen);

// 监听（处理了错误的情况）
int Listen(int fd,int backlog);

// accpet处理了错误的情况
int Accept(int listenfd,struct sockaddr* clientAddr,socklen_t* addrlen);

int ReadN(int fd,void* buf,int len);

int WriteN(int fd,const void* buf,int len);

int Close(int fd);

sockaddr_in CreateSockaddr(const char* ip,int port);

// 使用样例 -- begin
int SelectServer(const char* ip,int port);

int PSelectServer(const char* ip,int port);

int PollServer(const char* ip,int port);
// 使用样例 -- end

void Default_Sig_Chld(int sig);
void Default_Sig_Term(int sig);
void Default_Sig_Pipe(int sig);



#endif // SOCK_COMM_H
