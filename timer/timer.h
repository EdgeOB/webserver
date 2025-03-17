#ifndef TIMER_H
#define TIMER_H
#include <netinet/in.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
class util_timer;
struct client_data
{
    sockaddr_in address;
    int sockfd;
    util_timer *timer;
};

class util_timer{
public:
    util_timer(): pre(NULL), next(NULL){} 
    void (*cb_func)(client_data *);
public:
    time_t expire;
    client_data *userdata;
    util_timer *pre;
    util_timer *next;
};
class sort_timer{
public:
    sort_timer();
    ~sort_timer();

    void add_timer(util_timer *timer);
    void adjust_timer(util_timer *timer);
    void del_timer(util_timer *timer);
    void tick();
private:
    void add_timer(util_timer *timer, util_timer * t_head);
    util_timer *head;
    util_timer *tail;
};
class Utils{
public:
    Utils() {}
    ~Utils() {}

    void init(int timeslot);

    int setnonblocking(int fd);

    void addfd(int epfd, int fd, bool oneshot, int TRIGMode);

    static void sig_handler(int sig);

    void addsig(int sig, void(handler)(int), bool restart = true);

    void time_handler();

    void show_error(int connfd, const char *info);

public:

    static int *pipefd;
    sort_timer m_timer;
    static int m_epfd;
    int m_timeslot;
};

void cb_func(client_data * userdata);
#endif