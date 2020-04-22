/*
 *封装信号量、互斥量、条件变量
 */
#ifndef _LOCKER_H_
#define _LOCKER_H

#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>

/*信号量*/
class sem_locker
{
public:
	//初始化信号量
	sem_locker() {
		if (sem_init(&m_sem, 0, 0)) {
			printf("sem init error.\n");
		}
	}
	//销毁信号量
	~sem_locker() {
		sem_destroy(&m_sem);
	}
	//等待信号量
	bool wait() {
		return sem_wait(&m_sem) == 0;
	}
	//发布信号量
	bool add() {
		return sem_post(&m_sem) == 0;
	}
private:
	sem_t m_sem;  //信号量
};

/*互斥量*/
class mutex_locker
{
public:
	//初始化锁
	mutex_locker() {
		if (pthread_mutex_init(&m_mutex, NULL)) {
			printf("mutex init error.\n");
		}
	}
	//销毁锁
	~mutex_locker() {
		pthread_mutex_destroy(&m_mutex);
	}
	//lock mutex
	bool mutex_lock() {
		return pthread_mutex_lock(&m_mutex) == 0;
	}
	//unlock
	bool mutex_unlock() {
		return pthread_mutex_unlock(&m_mutex) == 0;
	}
private:
	pthread_mutex_t m_mutex;   //互斥量
};

//条件变量
class cond_locker
{
public:
	cond_locker() {
		if (pthread_mutex_init(&m_mutex, NULL)) {
			printf("mutex init error.\n");
		}
		if (pthread_cond_init(&m_cond, NULL)) {
			pthread_mutex_destroy(&m_mutex); //释放初始化成功的mutex
			printf("cond init error.\n");
		}
	}
	~cond_locker() {
		pthread_mutex_destroy(&m_mutex);
		pthread_cond_destroy(&m_cond);
	}
	//等待条件变量
	bool wait() {
		pthread_mutex_lock(&m_mutex);
		int reg = pthread_cond_wait(&m_cond, &m_mutex);
		pthread_mutex_unlock(&m_mutex);
		return reg;
	}
	//仅唤醒第一个睡眠等待条件变量的线程
	bool signal() {
		return pthread_cond_signal(&m_cond) == 0;
	}
	//唤醒所有等待条件变量的线程
	bool broadcast() {
		return pthread_cond_broadcast(&m_cond) == 0;
	}
private:
	pthread_mutex_t m_mutex;
	pthread_cond_t m_cond;
};
#endif // !_LOCKER_H_

