#ifndef _EPOLL_SERVER_H_
#define _EPOLL_SERVER_H_

#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/epoll.h>

#include "thread_pool.h"
#include "task.h"

#define MAX_EVENT 1024  //epoll_events的最大个数

class EpollServer
{
public:
	EpollServer() {};
	EpollServer(int ports, int thread_number); 
	~EpollServer();
	void init();  //EpollServer的初始化
	void epoll();
	static void setnonblocking(int fd);  //将fd设置为非阻塞
	//向Epoll中添加fd
	//oneshot表示是否设置成同一时刻，只能有一个线程访问fd，数据的读取都在主线程中，所以调用都设置成false
	static void addfd(int epollfd, int sockfd, bool oneshot);
private:
	bool is_stop;  //是否停止epool_wait的标志
	int thread_num; //线程池的数目
	int listenfd;  //监听的fd
	int port;  //端口
	int epollfd;  //epoll的fd
	threadpool<BaseTask> *pool;  //线程池的指针
	epoll_event events[MAX_EVENT];  //epoll的events数组
	struct sockaddr_in addr;  //绑定的套接字地址结构
};

EpollServer::EpollServer(int ports, int thread_number):
	is_stop(false),
	thread_num(thread_number),
	port(ports),
	pool(NULL)
{
	
}
EpollServer::~EpollServer()
{
	delete pool;
}

void EpollServer::init()
{
	//套接字地址结构的初始化
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//创建socket
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		printf("create socket error.\n");
		return;
	}
	int ret = bind(listenfd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		printf("bind error.\n");
		return;
	}
	ret = listen(listenfd, 10);
	if (ret < 0) {
		printf("listen error.\n");
		return;
	}
	//create epoll
	epollfd = epoll_create(MAX_EVENT);
	if (epollfd < 0) {
		printf("create epoll error.\n");
		return;
	}
	//创建线程池
	pool = new threadpool<BaseTask>(thread_num);
}

void EpollServer::epoll()
{
	pool->start(); //线程池启动
	addfd(epollfd, listenfd, false);
	while (!is_stop) {
		int ret = epoll_wait(epollfd, events, MAX_EVENT, -1);
		if (ret < 0) {
			printf("epoll wait error.\n");
			break;
		}
		for (int i = 0; i < ret; ++i) {
			int fd = events[i].data.fd;
			//新的连接
			if (fd == listenfd) { 
				struct sockaddr_in clientAddr;
				socklen_t len = sizeof(clientAddr);
				int confd = accept(listenfd, (struct sockaddr*)&clientAddr, &len);
				addfd(epollfd, confd, false);
			}
			else if (events[i].events & EPOLLIN) {
				char buffer[MAX_BUFFER];
readagain:		memset(buffer, 0, sizeof(buffer));
				int ret = read(fd, buffer, MAX_BUFFER - 1);
				//对端关闭了连接
				if (ret == 0) { 
					struct epoll_event ev;
					ev.events = EPOLLIN;
					ev.data.fd = fd;
					epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
					//终止网络连接的通用方法是调用close函数,但使用shutdown能更好的控制断连过程
					//HUT_RDWR：值为2，连接的读和写都关闭
					shutdown(fd, SHUT_RDWR);
					printf("%d logout.\n", fd);
					continue;
				}
				//出错
				else if (ret < 0) {
					if (errno == EAGAIN) {
						printf("read error.now read again.\n");
						goto readagain;
						break;
					}
				}
				//读取成功，向线程池中添加任务
				else {
					BaseTask *task = new Task(buffer, fd);
					pool->append_task(task);
				}
			}
			else {
				printf("something had happend.\n");
			}
		}
	}
	close(listenfd);
	pool->stop();
}

void EpollServer::setnonblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL); //获取文件描述符的标记
	flags |= O_NONBLOCK; //标记设置成非阻塞
	fcntl(fd, F_SETFL, flags); //更改文件描述符的标记
}

void EpollServer::addfd(int epollfd, int sockfd, bool oneshot)
{
	epoll_event event;
	event.data.fd = sockfd;
	event.events = EPOLLIN | EPOLLET;
	if (oneshot) {
		//EPOLLONESHOT：只监听一次事件
		//当监听完这次事件之后，如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里
		event.events |= EPOLLONESHOT;
	}
	epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event); //添加fd
	EpollServer::setnonblocking(sockfd);
}
#endif // !_EPOLL_SERVER_H_

