#ifndef _TASK_H_
#define _TASK_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER 2048  //Buffer的最大字节

class BaseTask
{
public:
	virtual void doit() = 0;
};

class Task :public BaseTask
{
public:
	Task(char *str, int fd) :
		sockfd(fd)
	{
		memset(order, 0, MAX_BUFFER); //某一块内存中的内容全部设置为指定的值
		strcpy(order, str);
	}
	//任务的执行函数
	void doit()
	{
		printf("data:%s\n", order);
		snprintf(order, MAX_BUFFER - 1, "somedata\n");
		write(sockfd, order, strlen(order));
	}
private:
	int sockfd;
	char order[MAX_BUFFER];
};
#endif // !_TASK_H_

