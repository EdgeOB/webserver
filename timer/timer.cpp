#include "timer.h"
#include "../http/http_conn.h"
sort_timer::sort_timer() {
    head = NULL;
    tail = NULL;
}

sort_timer::~sort_timer() {
    util_timer *temp = head;
    while(temp != NULL) {
        head = temp->next;
        delete temp;
        temp = head;
    }
}

void sort_timer::add_timer(util_timer *timer) {
    if(!timer) {
        return;
    }
    if(!head) {
        head = tail = timer;
        return;
    }
    if(timer->expire < head->expire) {
        timer->next = head;
        head->pre = timer;
        // timer->pre = NULL;
        head = timer;
        return;
    }
    add_timer(timer, head);
}
void sort_timer::adjust_timer(util_timer *timer) {
    /* if(!timer || head == tail || tail == timer) {
        return;
    }
    
    if(head == timer) {
        head = head->next;
    }
    else {
        util_timer *temp = head;
        while(temp != timer) {
            temp = temp->next;
        }
        temp->pre->next = temp->next;
        temp->next->pre = temp->pre;
    }
    add_timer(timer); */
    if (!timer)
    {
        return;
    }
    util_timer *tmp = timer->next;
    if (!tmp || (timer->expire < tmp->expire))
    {
        return;
    }
    if (timer == head)
    {
        head = head->next;
        head->pre = NULL;
        timer->next = NULL;
        add_timer(timer, head);
    }
    else
    {
        timer->pre->next = timer->next;
        timer->next->pre = timer->pre;
        add_timer(timer, timer->next);
    }
}
void sort_timer::del_timer(util_timer *timer) {
    /* if(timer == NULL) {
        return;
    }
    if(head == tail) {
        head = tail = NULL;
    }
    else if(head == timer) {
        head = head->next;
    }
    else if(tail == timer) {
        tail = tail->pre;
    }
    else {
        util_timer *temp = head;
        while(temp != timer) {
            temp = temp->next;
        }
        temp->pre->next = temp->next;
        temp->next->pre = temp->pre;
    }
    delete timer; */
    if (!timer)
    {
        return;
    }
    if ((timer == head) && (timer == tail))
    {
        delete timer;
        head = NULL;
        tail = NULL;
        return;
    }
    if (timer == head)
    {
        head = head->next;
        head->pre = NULL;
        delete timer;
        return;
    }
    if (timer == tail)
    {
        tail = tail->pre;
        tail->next = NULL;
        delete timer;
        return;
    }
    timer->pre->next = timer->next;
    timer->next->pre = timer->pre;
    delete timer;
}
void sort_timer::tick() {
    if(!head) {
        return;
    }
    time_t now = time(NULL);
    util_timer *temp = head;
    while(temp) {
        if(now < temp->expire) {
            break;
        }
        temp->cb_func(temp->userdata);
        head = temp->next;
        if(head) {
            head->pre = NULL;
        }
        delete temp;
        temp = head;
    }
}
void sort_timer::add_timer(util_timer *timer, util_timer *t_head) {
    /* util_timer *temp;
    while(t_head && t_head->expire < timer->expire) {
        temp = t_head;
        t_head = t_head->next;
    }
    timer->next = temp->next;
    temp->next = timer;
    timer->pre = temp;
    if(timer->next) {
        timer->next->pre = timer;
    }
    else{
        tail = timer;
    } */
    util_timer *prev = t_head;
    util_timer *tmp = prev->next;
    while (tmp)
    {
        if (timer->expire < tmp->expire)
        {
            prev->next = timer;
            timer->next = tmp;
            tmp->pre = timer;
            timer->pre = prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    if (!tmp)
    {
        prev->next = timer;
        timer->pre = prev;
        timer->next = NULL;
        tail = timer;
    }
}

void Utils::init(int timeslot) {
    m_timeslot = timeslot;
}
//将事件fd设置为非阻塞
int Utils::setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void Utils::addfd(int epfd, int fd, bool oneshot, int TRIGMode) {
    epoll_event event;
    event.data.fd = fd;

    if(1 == TRIGMode) {
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    }
    else {
        event.events = EPOLLIN | EPOLLRDHUP;
    }
    if(oneshot) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);

    setnonblocking(fd);
}

// ?
void Utils::sig_handler(int sig) {

    int save_errno = errno;
    int msg = sig;
    send(pipefd[1], (char *)(&msg), 1, 0);
    errno = save_errno;
}

void Utils::addsig(int sig, void(handler)(int), bool restart) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if(restart) {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void Utils::time_handler() {
    m_timer.tick();
    alarm(m_timeslot);
}

void Utils::show_error(int connfd, const char *info) {
    send(connfd, info, sizeof(info), 0);
    close(connfd);
}

int *Utils::pipefd = 0;
int Utils::m_epfd = 0;

class Utils;
void cb_func(client_data *userdata) {
    epoll_ctl(Utils::m_epfd, EPOLL_CTL_DEL, userdata->sockfd, NULL);

    assert(userdata);

    close(userdata->sockfd);

    http_conn::m_user_count--;
}