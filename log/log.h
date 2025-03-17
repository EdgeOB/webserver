#ifndef LOG_H
#define LOG_H
#include "block_queue.h"
#include <stdio.h>
#include <string>
#include <iostream>
#include "../lock/locker.h"
using namespace std;
class Log{
public:
    //懒汉单例
    static Log* get_instance() {
        static Log instance;
        return &instance;
    }
    static void *flush_log_buf(void *arg) {
        Log::get_instance()->async_write_log();
    }

    bool init(const char *file_name, int close_log, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);

    void write_log(int level, const char *format, ...);
    
    void flush(void);

private:
    Log();
    virtual ~Log();
    void *async_write_log() {
        string single_log;
        
        while(m_log_queue->pop(single_log)) {
            m_mutex.lock();
            fputs(single_log.c_str(), fp);
            m_mutex.unlock();
        }
        return NULL;
    }

private:
    char dir_name[128];
    char log_name[128];
    int m_max_lines;    //单个日志最大行数
    int m_log_buf_size;
    long long m_count;    //日志总行数
    int m_today;    //记录当前时间那一天
    FILE *fp;
    char *buf;
    block_queue<string> *m_log_queue;
    bool m_is_async;
    locker m_mutex;
    int m_close_log;
};
#define LOG_DEBUG(format, ...) if(0 == m_close_log){Log::get_instance()->write_log(0, format,##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_INFO(format, ...) if(0 == m_close_log){Log::get_instance()->write_log(1, format,##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_WARN(format, ...) if(0 == m_close_log){Log::get_instance()->write_log(2, format,##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_ERROR(format, ...) if(0 == m_close_log){Log::get_instance()->write_log(3, format,##__VA_ARGS__); Log::get_instance()->flush();}

#endif