/*
 *��װ�ź���������������������
 */
#ifndef _LOCKER_H_
#define _LOCKER_H

#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>

/*�ź���*/
class sem_locker
{
public:
	//��ʼ���ź���
	sem_locker() {
		if (sem_init(&m_sem, 0, 0)) {
			printf("sem init error.\n");
		}
	}
	//�����ź���
	~sem_locker() {
		sem_destroy(&m_sem);
	}
	//�ȴ��ź���
	bool wait() {
		return sem_wait(&m_sem) == 0;
	}
	//�����ź���
	bool add() {
		return sem_post(&m_sem) == 0;
	}
private:
	sem_t m_sem;  //�ź���
};

/*������*/
class mutex_locker
{
public:
	//��ʼ����
	mutex_locker() {
		if (pthread_mutex_init(&m_mutex, NULL)) {
			printf("mutex init error.\n");
		}
	}
	//������
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
	pthread_mutex_t m_mutex;   //������
};

//��������
class cond_locker
{
public:
	cond_locker() {
		if (pthread_mutex_init(&m_mutex, NULL)) {
			printf("mutex init error.\n");
		}
		if (pthread_cond_init(&m_cond, NULL)) {
			pthread_mutex_destroy(&m_mutex); //�ͷų�ʼ���ɹ���mutex
			printf("cond init error.\n");
		}
	}
	~cond_locker() {
		pthread_mutex_destroy(&m_mutex);
		pthread_cond_destroy(&m_cond);
	}
	//�ȴ���������
	bool wait() {
		pthread_mutex_lock(&m_mutex);
		int reg = pthread_cond_wait(&m_cond, &m_mutex);
		pthread_mutex_unlock(&m_mutex);
		return reg;
	}
	//�����ѵ�һ��˯�ߵȴ������������߳�
	bool signal() {
		return pthread_cond_signal(&m_cond) == 0;
	}
	//�������еȴ������������߳�
	bool broadcast() {
		return pthread_cond_broadcast(&m_cond) == 0;
	}
private:
	pthread_mutex_t m_mutex;
	pthread_cond_t m_cond;
};
#endif // !_LOCKER_H_

