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

#define MAX_EVENT 1024  //epoll_events��������

class EpollServer
{
public:
	EpollServer() {};
	EpollServer(int ports, int thread_number); 
	~EpollServer();
	void init();  //EpollServer�ĳ�ʼ��
	void epoll();
	static void setnonblocking(int fd);  //��fd����Ϊ������
	//��Epoll�����fd
	//oneshot��ʾ�Ƿ����ó�ͬһʱ�̣�ֻ����һ���̷߳���fd�����ݵĶ�ȡ�������߳��У����Ե��ö����ó�false
	static void addfd(int epollfd, int sockfd, bool oneshot);
private:
	bool is_stop;  //�Ƿ�ֹͣepool_wait�ı�־
	int thread_num; //�̳߳ص���Ŀ
	int listenfd;  //������fd
	int port;  //�˿�
	int epollfd;  //epoll��fd
	threadpool<BaseTask> *pool;  //�̳߳ص�ָ��
	epoll_event events[MAX_EVENT];  //epoll��events����
	struct sockaddr_in addr;  //�󶨵��׽��ֵ�ַ�ṹ
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
	//�׽��ֵ�ַ�ṹ�ĳ�ʼ��
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//����socket
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
	//�����̳߳�
	pool = new threadpool<BaseTask>(thread_num);
}

void EpollServer::epoll()
{
	pool->start(); //�̳߳�����
	addfd(epollfd, listenfd, false);
	while (!is_stop) {
		int ret = epoll_wait(epollfd, events, MAX_EVENT, -1);
		if (ret < 0) {
			printf("epoll wait error.\n");
			break;
		}
		for (int i = 0; i < ret; ++i) {
			int fd = events[i].data.fd;
			//�µ�����
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
				//�Զ˹ر�������
				if (ret == 0) { 
					struct epoll_event ev;
					ev.events = EPOLLIN;
					ev.data.fd = fd;
					epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
					//��ֹ�������ӵ�ͨ�÷����ǵ���close����,��ʹ��shutdown�ܸ��õĿ��ƶ�������
					//HUT_RDWR��ֵΪ2�����ӵĶ���д���ر�
					shutdown(fd, SHUT_RDWR);
					printf("%d logout.\n", fd);
					continue;
				}
				//����
				else if (ret < 0) {
					if (errno == EAGAIN) {
						printf("read error.now read again.\n");
						goto readagain;
						break;
					}
				}
				//��ȡ�ɹ������̳߳����������
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
	int flags = fcntl(fd, F_GETFL); //��ȡ�ļ��������ı��
	flags |= O_NONBLOCK; //������óɷ�����
	fcntl(fd, F_SETFL, flags); //�����ļ��������ı��
}

void EpollServer::addfd(int epollfd, int sockfd, bool oneshot)
{
	epoll_event event;
	event.data.fd = sockfd;
	event.events = EPOLLIN | EPOLLET;
	if (oneshot) {
		//EPOLLONESHOT��ֻ����һ���¼�
		//������������¼�֮���������Ҫ�����������socket�Ļ�����Ҫ�ٴΰ����socket���뵽EPOLL������
		event.events |= EPOLLONESHOT;
	}
	epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event); //���fd
	EpollServer::setnonblocking(sockfd);
}
#endif // !_EPOLL_SERVER_H_

