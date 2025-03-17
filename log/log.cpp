#include "log.h"
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <stdarg.h>
#include <iostream>
using namespace std;

Log::Log() {
    m_count = 0;

    m_is_async = false;
}

Log::~Log() {
    if(fp != NULL) {
        fclose(fp);
    }
}

bool Log::init(const char *file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size) {
    // 异步模式初始化（队列大小≥1时启用）
    if(max_queue_size >= 1) {
        m_is_async = true;
        // 创建异步日志队列
        m_log_queue = new block_queue<string>(max_queue_size);
        
        // 创建日志刷新线程
        pthread_t tid;
        pthread_create(&tid, NULL, flush_log_buf, NULL);
    }

    m_close_log = close_log;
    m_log_buf_size = log_buf_size;
    buf = new char[m_log_buf_size];
    memset(buf, '\0', sizeof(buf));
    m_max_lines = split_lines;

    time_t now = time(NULL);
    struct tm *sys_time = localtime(&now);
    struct tm my_time = *sys_time;

    const char *p = strrchr(file_name,'/');

    char log_full_name[256] = {0};

    // 生成日期化日志文件名（按天分割）
    if(p == NULL) {
        snprintf(log_full_name,255,"%d_%02d_%02d_%s",my_time.tm_year + 1900, my_time.tm_mon + 1, my_time.tm_mday, file_name);
    }
    else {
        strcpy(log_name, p + 1);
        strncpy(dir_name, file_name, p - file_name + 1);
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name,my_time.tm_year + 1900, my_time.tm_mon + 1, my_time.tm_mday, log_name);
    }

    m_today = my_time.tm_mday;

    // 打开日志文件（追加模式）
    fp = fopen(log_full_name, "a");

    if(fp == NULL) {
        return false;
    }

    return true;
}

void Log::write_log(int level, const char *format, ...) {
    if(!fp) {
        return;
    }
    struct timeval t = {0, 0};
    gettimeofday(&t, NULL);
    time_t now = t.tv_sec;

    struct tm *sys_tm = localtime(&now);
    struct tm my_tm = *sys_tm;

    char s[16] = {0};
    switch(level) {
        case 0:
            strcpy(s, "[debug]:");
            break;
        case 1:
            strcpy(s, "[info]:");
            break;
        case 2:
            strcpy(s, "[warn]:");
            break;
        case 3:
            strcpy(s, "[error]:");
            break;
        default:
            strcpy(s, "[info]:");
            break;
    }

    // 日志文件轮换逻辑（跨天或达到最大行数）
    m_mutex.lock();
    m_count++;
    if(my_tm.tm_mday != m_today || m_count % m_max_lines == 0) {
        // 生成新文件名：日期变化时重置计数器，行数超限时添加序号
        char new_log[256];
        fflush(fp);
        fclose(fp);
        char s[16];
        snprintf(s, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);
        if(my_tm.tm_mday != m_today) {
            snprintf(new_log, 256, "%s%s%s", dir_name, s, log_name);
            m_today = my_tm.tm_mday;
            m_count = 0;
        }
        else {
            snprintf(new_log, 256,"%s%s%s.%lld",dir_name,s,log_name, m_count / m_max_lines);
        }
        fp = fopen(new_log, "a");
    }

    m_mutex.unlock();

    va_list valist;
    va_start(valist, format);

    string log_str;

    m_mutex.lock();

    int n = snprintf(buf,48,"%d-%02d-%02d: %02d:%02d:%02d.%06ld %s", 
        my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
        my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, t.tv_usec,s);
    
    int m = vsnprintf(buf + n, m_log_buf_size - n - 1, format, valist);
    
    buf[n + m] = '\n';
    buf[n + m + 1] = '\0';

    log_str = buf;

    m_mutex.unlock();
    // 选择同步/异步写入方式
    if(m_is_async && !m_log_queue->full()) {
        m_log_queue->push(log_str);  // 异步写入队列
    }
    else {
        m_mutex.lock();
        fputs(buf, fp);  // 同步直接写入文件
        m_mutex.unlock();
    }
    va_end(valist); //释放valist指针
}
void Log::flush(void) {
    m_mutex.lock();
    fflush(fp);
    m_mutex.unlock();
}