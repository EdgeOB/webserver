#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>
/* lock分别通过信号量、互斥锁、以及条件变量来保证
    线程之间的对于共享资源的互斥访问 */

//用来控制访问共享资源
class sem{
public:
    sem() {
        if(sem_init(&m_sem, 0, 0) != 0) {
            throw std::exception();
        }
    }
    sem(int num) {
        if(sem_init(&m_sem, 0 ,num) != 0) {
            throw std::exception();
        }
    }
    ~sem() {
        sem_destroy(&m_sem);
    }
    bool wait() {
        return sem_wait(&m_sem) == 0;
    }
    bool post() {
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem;
};
class locker{
public:
    locker() {
        if(pthread_mutex_init(&mutex, NULL) != 0) {
            throw std::exception();
        }
    }
    ~locker() {
        pthread_mutex_destroy(&mutex);
    }
    bool lock() {
        return pthread_mutex_lock(&mutex) == 0;
    }
    bool unlock() {
        return pthread_mutex_unlock(&mutex) == 0;
    }
    pthread_mutex_t* get() {
        return &mutex;
    }
private:
    pthread_mutex_t mutex;
};

class cond{
public:
    cond() {
        if(pthread_cond_init(&m_cond, NULL) != 0) {
            throw std::exception();
        } 
    }
    ~cond() {
        pthread_cond_destroy(&m_cond);
    }
    bool wait(pthread_mutex_t *mutex) {
        int ret = 0;
        ret = pthread_cond_wait(&m_cond, mutex);
        return ret == 0;
    }
    bool timewait(pthread_mutex_t *mutex, struct timespec t) {
        int ret = 0;
        ret = pthread_cond_timedwait(&m_cond, mutex, &t);
        return ret == 0;
    }
    bool signal() {
        return pthread_cond_signal(&m_cond) == 0;
    }
    bool broadcast() {
        return pthread_cond_broadcast(&m_cond) == 0;
    }
private:
    pthread_cond_t m_cond;
};

#endif