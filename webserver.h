#ifndef WEBSERVER_H
#define WEBSERVER_H
#include <string>
#include <arpa/inet.h>
#include "timer/timer.h"
#include "http/http_conn.h"
#include "threadpool/threadpool.h"
using namespace std;

const int MAX_FD = 65536;
const int MAX_EVENT = 10000;
const int TIMESLOT = 5; //最小超时单位

class webserver{
public:
    webserver();
    ~webserver();

    void init(int port, string user, string passWord, string databaseName, int log_write,
              int opt_linger, int trigmode, int sql_num, int thread_num, int close_log, int actor_model);
    
    void thread_pool();
    void sql_pool();
    void log_write();
    void trig_mode();
    void event_listen();
    void event_loop();
    void timer(int connfd, struct sockaddr_in client_address);
    void adjust_timer(util_timer *timer);
    void deal_timer(util_timer *timer, int sockfd);
    bool dealclientdata();
    bool dealwithsignal(bool &timeout, bool &stop_server);
    void dealwithread(int sockfd);
    void dealwithwirte(int sockfd);

public:
    
    int m_port;
    char *m_root;
    int m_log_write;
    int m_close_log;
    int m_actormodel;   //服务器并发模式
    
    int m_pipefd[2];
    int m_epfd;
    http_conn *users;

    //database
    connection_pool *m_connpool;
    string m_user;
    string m_passwd;
    string m_databasename;
    int m_sql_num;

    //thread
    threadpool<http_conn> *m_threadpool;
    int m_thread_num;

    epoll_event event[MAX_EVENT];

    int m_listenfd;
    int m_opt_linger;       //控制套接字关闭时的行为，例如是否等待数据发送完毕
    int m_TRIGMode;
    int m_listen_mode;
    int m_conn_mode;

    //定时器相关
    client_data *users_timer;
    Utils utils;
};
#endif