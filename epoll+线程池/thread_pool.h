#ifndef _PTHREAD_POOL_
#define _PTHREAD_POOL_

#include <queue>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <iostream>

#include "locker.h"

template <typename T>
class threadpool
{
public:
	threadpool(int thread_num = 20);  //构造含有指定数量线程的线程池
	~threadpool();
	void start();  //线程池开启
	void stop();  //线程池关闭
	bool append_task(T *task);  //添加任务
private:
	static void* worker(void* arg);  //线程运行的函数
	void run();
	T* getTask(); //run()中获取一个任务

	int thread_number;  //线程池的线程数
	pthread_t *all_threads; //线程数组
	std::queue<T *> task_queue; //任务队列
	mutex_locker queue_mutex_locker;  //互斥锁
	cond_locker queue_cond_locker; //条件变量
	bool is_stop; //是否结束线程池的运作
};

/*构造含有指定数量线程的线程池*/
template<typename T>
threadpool<T>::threadpool(int thread_num):
	thread_number(thread_num),
	is_stop(false),
	all_threads(NULL)
{
	if (thread_num <= 0) {
		printf("threadpool init error:thread's number =0.\n");
	}
	all_threads = new pthread_t[thread_number];
	if (all_threads == NULL) {
		printf("threadpool init error:thread array can't new.\n");
	}
}

template<typename T>
threadpool<T>::~threadpool()
{
	delete[] all_threads;
	stop();
}
/*线程池开启*/
template<typename T>
void threadpool<T>::start()
{
	for (int i = 0; i < thread_number; ++i) {
		if (pthread_create(all_threads + i, NULL, worker, this)) {
			delete[] all_threads;
		}
		//将线程设置为unjoinable状态，即脱离主线程，函数结束后自动回收资源
		if (pthread_detach(all_threads[i])) {
			delete[] all_threads;
		}
	}
}
/*线程池关闭*/
template<typename T>
void threadpool<T>::stop()
{
	is_stop = true;
	queue_cond_locker.broadcast(); //唤醒所有睡眠等待的线程
}
/*向任务队列中添加一个任务*/
template<typename T>
bool threadpool<T>::append_task(T *task)
{
	queue_mutex_locker.mutex_lock();  //获取互斥锁
	bool is_signal = task_queue.empty();
	task_queue.push(task); //添加任务
	queue_mutex_locker.mutex_unlock();
	//唤醒等待任务的一个线程,is_signal=true：添加任务之前任务队列为空，则可能有线程在等待
	if (is_signal) {
		queue_cond_locker.signal();
	}
	return true;
}
/*工作函数*/
template<typename T>
void* threadpool<T>::worker(void* arg)
{
	//woeker是static成员，没有this指针，因此通过参数传threadpool对象的this指针
	threadpool *pool = (threadpool *)arg; 
	pool->run();
	return pool;
}
/*从任务队列中获取一个任务*/
template<typename T>
T * threadpool<T>::getTask()
{
	T *task = NULL;
	queue_mutex_locker.mutex_lock();
	if (!task_queue.empty()) {
		task = task_queue.front();
		task_queue.pop();
	}
	queue_mutex_locker.mutex_unlock();
	return task;
}

template<typename T>
void threadpool<T>::run()
{
	while (!is_stop) {
		T *task = getTask();
		if (task == NULL) { //若任务队列为空
			queue_cond_locker.wait();
		}
		else {
			task->doit();
		}
	}
}
#endif // !_PTHREAD_POOL_

