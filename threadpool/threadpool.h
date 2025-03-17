#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <exception>
#include "../CGImysql/sql_connection_pool.h"
using namespace std;
template<typename T>
class threadpool{
public:
    threadpool(int actor_model, connection_pool *connpool, int thread_number = 8, int max_request = 10000);
    ~threadpool();
    bool append(T *request, int state);
    bool append(T *request);

private:
    static void *worker(void *arg);
    void run();

private:
    int m_actor_model;
    connection_pool *m_connpool;
    int m_thread_number;
    int m_max_request;

    pthread_t *m_threads;
    list<T *> m_worker_queue;
    locker m_mutex;
    sem quest_stat;

};
template<typename T>
threadpool<T>::threadpool(int actor_model, connection_pool *connpool, int thread_number, int max_request): m_actor_model(actor_model), m_connpool(connpool), m_thread_number(thread_number), m_max_request(max_request), m_threads(NULL){
    if(thread_number <= 0 || max_request <= 0) {
        throw exception();
    }
    m_threads = new pthread_t[m_thread_number];
    if(!m_threads) {
        throw exception();
    }
    for(int i = 0; i < m_thread_number; ++i) {
        if(pthread_create(m_threads + i, NULL, worker, this)) {
            delete[] m_threads;
            throw exception();
        }
        //将线程设置为脱离线程，线程结束后自动释放所有资源
        if(pthread_detach(m_threads[i])) {
            delete[] m_threads;
            throw exception();
        }
    }
}
template<typename T>
threadpool<T>::~threadpool() {
    delete[] m_threads;
}

template<typename T>
bool threadpool<T>::append(T *request, int state) {
    m_mutex.lock();
    if(m_worker_queue.size() >= m_max_request) {
        m_mutex.unlock();
        return false;
    }
    request->m_state = state;
    m_worker_queue.push_back(request);
    m_mutex.unlock();
    quest_stat.post();
    return true;
}

template<typename T>
bool threadpool<T>::append(T *request) {
    m_mutex.lock();
    if(m_worker_queue.size() >= m_max_request) {
        m_mutex.unlock();
        return false;
    }
    m_worker_queue.push_back(request);
    m_mutex.unlock();
    quest_stat.post();
    return true;
}
template<typename T>
void *threadpool<T>::worker(void *arg) {
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return pool;
}
//线程，完成http后回来再看。
template<typename T>
void threadpool<T>::run() {
    while(true) {
        quest_stat.wait();
        m_mutex.lock();
        if(m_worker_queue.size() == 0) {
            m_mutex.unlock(); 
            continue;
        }

        T *request = m_worker_queue.front();
        m_worker_queue.pop_front();
        m_mutex.unlock();

        if(!request) {
            continue;
        }
        //reactor
        if(1 == m_actor_model) {

            if(0 == request->m_state) {
                if(request->read_once()) {
                    request->improv = 1;
                    connectionRAII conRAII(&request->mysql, m_connpool);
                    request->process();
                }
                else {
                    request->improv = 1;
                    request->time_flag = 1;
                }
            }
            else {
                if(request->write()) {
                    request->improv = 1;
                }
                else {
                    request->improv = 1;
                    request->time_flag = 1;
                }G
            }
        }
        //proactor
        else {
            connectionRAII conRAII(&request->mysql, m_connpool);//从连接池中取出一条连接交给conRAII管理
            request->process();
        }
    }
    
}
#endif