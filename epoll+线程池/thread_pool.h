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
	threadpool(int thread_num = 20);  //���캬��ָ�������̵߳��̳߳�
	~threadpool();
	void start();  //�̳߳ؿ���
	void stop();  //�̳߳عر�
	bool append_task(T *task);  //�������
private:
	static void* worker(void* arg);  //�߳����еĺ���
	void run();
	T* getTask(); //run()�л�ȡһ������

	int thread_number;  //�̳߳ص��߳���
	pthread_t *all_threads; //�߳�����
	std::queue<T *> task_queue; //�������
	mutex_locker queue_mutex_locker;  //������
	cond_locker queue_cond_locker; //��������
	bool is_stop; //�Ƿ�����̳߳ص�����
};

/*���캬��ָ�������̵߳��̳߳�*/
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
/*�̳߳ؿ���*/
template<typename T>
void threadpool<T>::start()
{
	for (int i = 0; i < thread_number; ++i) {
		if (pthread_create(all_threads + i, NULL, worker, this)) {
			delete[] all_threads;
		}
		//���߳�����Ϊunjoinable״̬�����������̣߳������������Զ�������Դ
		if (pthread_detach(all_threads[i])) {
			delete[] all_threads;
		}
	}
}
/*�̳߳عر�*/
template<typename T>
void threadpool<T>::stop()
{
	is_stop = true;
	queue_cond_locker.broadcast(); //��������˯�ߵȴ����߳�
}
/*��������������һ������*/
template<typename T>
bool threadpool<T>::append_task(T *task)
{
	queue_mutex_locker.mutex_lock();  //��ȡ������
	bool is_signal = task_queue.empty();
	task_queue.push(task); //�������
	queue_mutex_locker.mutex_unlock();
	//���ѵȴ������һ���߳�,is_signal=true���������֮ǰ�������Ϊ�գ���������߳��ڵȴ�
	if (is_signal) {
		queue_cond_locker.signal();
	}
	return true;
}
/*��������*/
template<typename T>
void* threadpool<T>::worker(void* arg)
{
	//woeker��static��Ա��û��thisָ�룬���ͨ��������threadpool�����thisָ��
	threadpool *pool = (threadpool *)arg; 
	pool->run();
	return pool;
}
/*����������л�ȡһ������*/
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
		if (task == NULL) { //���������Ϊ��
			queue_cond_locker.wait();
		}
		else {
			task->doit();
		}
	}
}
#endif // !_PTHREAD_POOL_

