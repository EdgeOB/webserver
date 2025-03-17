#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include "../lock/locker.h"
#include <stdlib.h>
#include <sys/time.h>
template<typename T>
class block_queue{
public:
    block_queue(int max_size = 1000) {
        if(max_size <= 0) {
            exit(-1);
        }
        m_max_size = max_size;
        m_array = new T[max_size];
        m_size = 0;
        m_front = -1;
        m_back = -1;
    }
    
    void clear() {
        m_mutex.lock();
        m_size = 0;
        m_front = -1;
        m_back = -1;
        m_mutex.unlock();
    }

    ~block_queue() {
        m_mutex.lock();
        if(m_array != NULL)
            delete[] m_array;
        m_mutex.unlock();
    }

    bool full() {
        m_mutex.lock();
        if(m_size >= m_max_size) {
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }

    bool empty() {
        m_mutex.lock();
        if(m_size == 0) {
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }

    bool front(T &value) {
        m_mutex.lock();
        if(0 == m_size) {
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_front];
        m_mutex.unlock();
        return true;
    }
    bool back(T &value) {
        m_mutex.lock();
        if(0 == m_size) {
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_back];
        m_mutex.unlock();
        return true;
    }
    int size() {
        int temp = 0;
        m_mutex.lock();
        temp = m_size;
        m_mutex.unlock();
        return temp;
    }
    int max_size() {
        int temp = 0;
        m_mutex.lock();
        temp = m_max_size;
        m_mutex.unlock();
        return temp;
    }

    bool push(const T &item) {
        m_mutex.lock();
        if(m_size >= m_max_size) {
            m_cond.broadcast();
            m_mutex.unlock();
            return false;
        }
        m_back = (m_back + 1) % m_max_size;
        m_array[m_back] = item;
        ++m_size;
        //唤醒由于队列为空的阻塞的pop中的wait
        m_cond.broadcast();
        
        m_mutex.unlock();
        return true;
    }

    bool pop(T &item) {
        m_mutex.lock();
        while(m_size <= 0) {

            if(!m_cond.wait(m_mutex.get())) {
                m_mutex.unlock();
                return false;
            }
        }
        m_front = (m_front + 1) % m_max_size;
        item = m_array[m_front];
        --m_size;
        m_mutex.unlock();
        return true;
    }
    //超时处理
    bool pop(T &item, int ms_timeout) {
        struct timespec t = {0, 0};
        struct timeval now = {0, 0};
        gettimeofday(&now,NULL);
        m_mutex.lock();
        if(m_size <= 0) {
            
            t.tv_sec = now.tv_sec + ms_timeout / 1000;  //秒
            t.tv_nsec = (ms_timeout % 1000) * 1000;     //纳秒
            if(!m_cond.timewait(m_mutex.get(), t)) {
                m_mutex.unlock();
                return false;
            }
        }
        if(m_size <= 0) {
            m_mutex.unlock();
            return false;
        }
        m_front = (m_front + 1) % m_max_size;
        item = m_array[m_front];
        --m_size;
        m_mutex.unlock();
        return true;
    }
private:
    T *m_array;
    locker m_mutex;
    cond m_cond;

    int m_max_size;
    int m_size;
    int m_front;
    int m_back;
};


#endif