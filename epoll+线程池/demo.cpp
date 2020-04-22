#include "epoll_server.h"

#define PORT 8000
#define THREAD_NUM 20
int main()
{
	EpollServer *epoll = new EpollServer(PORT, THREAD_NUM);
	epoll->init();
	epoll->epoll();
    return 0;
}