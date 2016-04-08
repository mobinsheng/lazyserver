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

int main()
{
    sockaddr_in addr;
    bzero(&addr,sizeof(sockaddr_in));

    addr.sin_family = AF_INET;
    inet_pton(AF_INET,"127.0.0.1",&addr.sin_addr);
    addr.sin_port = htons(8888);

    int fd = socket(PF_INET,SOCK_STREAM,0);
    connect(fd,(sockaddr*)&addr,sizeof(addr));
    int count = 0;
    while( count < 10240)
    {

        string str = "sockaddr_in addr;bzero(&addr,sizeof(sockaddr_in));addr.sin_family = AF_INET;addr.sin_port = htons(8888);int fd = socket(PF_INET,SOCK_STREAM,0);";
        send(fd,str.c_str(),str.size(),0);
       
       usleep(10);
        ++count;
    }
    cout << "Hello World!" << endl;
    
    
    return 0;
}


