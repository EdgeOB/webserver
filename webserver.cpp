# include"webserver.h"


using namespace std;

webserver::webserver() {
    users = new http_conn[MAX_FD];

    char server_path[200];
    getcwd(server_path, 200);
    char root[6] = "/root";
    m_root = (char *)malloc(strlen(server_path) + strlen(root) + 1);
    strcpy(m_root, server_path);
    strcat(m_root, root);

    users_timer = new client_data[MAX_FD];
}
//为什么不delete m_connpool ?
webserver::~webserver(){
    close(m_epfd);
    close(m_listenfd);
    close(m_pipefd[1]);
    close(m_pipefd[0]);
    
    

    delete[] users;
    delete[] users_timer;
    delete m_threadpool;
}

void webserver::init(int port, string user, string passWord, string databaseName, int log_write,
                     int opt_linger, int trigmode, int sql_num, int thread_num, int close_log, int actor_model) {
    m_port = port;
    m_user = user;
    m_passwd = passWord;
    m_databasename = databaseName;
    m_log_write = log_write;
    m_opt_linger = opt_linger;
    m_TRIGMode = trigmode;
    m_sql_num = sql_num;
    m_thread_num = thread_num;
    m_close_log = close_log;
    m_actormodel = actor_model;
}
//确定监听和连接的事件通知机制
void webserver::trig_mode() {
    // m_listen_mode = m_TRIGMode >> 1 & 1;
    // m_conn_mode = m_TRIGMode & 1;
    //LT + LT
    if (0 == m_TRIGMode)
    {
        m_listen_mode = 0;
        m_conn_mode = 0;
    }
    //LT + ET
    else if (1 == m_TRIGMode)
    {
        m_listen_mode = 0;
        m_conn_mode = 1;
    }
    //ET + LT
    else if (2 == m_TRIGMode)
    {
        m_listen_mode = 1;
        m_conn_mode = 0;
    }
    //ET + ET
    else if (3 == m_TRIGMode)
    {
        m_listen_mode = 1;
        m_conn_mode = 1;
    }
}

void webserver::log_write() {
    if(0 == m_close_log) {
        if(1 == m_log_write) {
            Log::get_instance()->init("./yzfLog", m_close_log, 2000, 800000, 800);
        }
        else {
            Log::get_instance()->init("./yzfLog", m_close_log, 2000, 800000, 0);
        }
    }

}

void webserver::sql_pool() {
    m_connpool = connection_pool::getInstance();
    m_connpool->init("localhost",m_user,m_passwd,m_databasename,3306, m_sql_num, m_close_log);
    //通过第一个http_conn对象初始化map<string, string> users，这是一个非成员变量
    users->initmysql_result(m_connpool);
}

void webserver::thread_pool() {
    
    //线程池
    m_threadpool = new threadpool<http_conn>(m_actormodel, m_connpool, m_thread_num);
}

void webserver::event_listen() {

    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);

    assert(m_listenfd >= 0);

    if(0 == m_opt_linger) {
        struct linger tmp = {0, 1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }
    else if(1 == m_opt_linger) {
        struct linger tmp = {1, 1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }

    int ret = 0;
    struct sockaddr_in addr;
    // memset(&addr, 0, sizeof(addr));
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int flag = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ret = bind(m_listenfd, (struct sockaddr*)&addr, sizeof(addr));
    assert(ret >= 0);
    ret = listen(m_listenfd, 5);
    assert(ret >= 0);

    utils.init(TIMESLOT);

    m_epfd = epoll_create(5);
    assert(m_epfd != -1);

    utils.addfd(m_epfd, m_listenfd, false, m_listen_mode);
    http_conn::m_epfd = m_epfd;

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);
    assert(ret != -1);

    utils.setnonblocking(m_pipefd[1]);
    utils.addfd(m_epfd, m_pipefd[0], false, 0);

    utils.addsig(SIGPIPE, SIG_IGN);     //ignore
    utils.addsig(SIGALRM, utils.sig_handler, false);    
    utils.addsig(SIGTERM, utils.sig_handler, false);

    alarm(TIMESLOT);

    Utils::m_epfd = m_epfd;
    Utils::pipefd = m_pipefd;
}

void webserver::timer(int connfd, struct sockaddr_in client_address) {
    users[connfd].init(connfd,client_address,m_root,m_conn_mode,m_close_log,m_user,m_passwd,m_databasename);

    users_timer[connfd].address = client_address;
    users_timer[connfd].sockfd = connfd;
    util_timer *timer = new util_timer;
    timer->userdata = &users_timer[connfd];
    timer->cb_func = cb_func;
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;
    users_timer[connfd].timer = timer;
    utils.m_timer.add_timer(timer);
}

void webserver::adjust_timer(util_timer *timer) {
    time_t now = time(NULL);
    timer->expire = now + 3 * TIMESLOT;
    utils.m_timer.adjust_timer(timer);

    LOG_INFO("%s", "adjust timer once");
}

void webserver::deal_timer(util_timer *timer, int sockfd) {
    timer->cb_func(&users_timer[sockfd]);

    if(timer) {
        utils.m_timer.del_timer(timer);
    }

    LOG_INFO("close fd:%d", sockfd);    //冗余
}

bool webserver::dealclientdata() {
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    if(0 == m_listen_mode) {
        int connfd = accept(m_listenfd,(struct sockaddr*)&client_address, &client_addrlength);
        //connfd就是通过http_conn init加入epfd的监听集合当中
        if(connfd < 0) {
            LOG_ERROR("%s:errno is %d","accept error", errno);
            return false;
        }
        if(http_conn::m_user_count >= MAX_FD) {
            utils.show_error(connfd, "Internal server busy");
            LOG_ERROR("%s", "Internal server busy");
            return false;
        }
        LOG_INFO("new client:%s:%d, fd:%d", inet_ntoa(client_address.sin_addr), client_address.sin_port, connfd);
        timer(connfd, client_address);//将connfd添加到时钟序列，并且初始化http_conn
    }
    else {
        //ET
        while(1) {
            int connfd = accept(m_listenfd,(struct sockaddr*)&client_address, &client_addrlength);
            if(connfd < 0) {
                LOG_ERROR("%s:errno is %d","accept error", errno);
                // return false;
                break;
            }
            if(http_conn::m_user_count >= MAX_FD) {
                utils.show_error(connfd, "Internal server busy");
                LOG_ERROR("%s", "Internal server busy");
                // return false;
                break;
            }
            timer(connfd, client_address);
        }
        return false;
    }
    return true;
}

bool webserver::dealwithsignal(bool &timeout, bool &stop_server) {
    int ret = 0;
    int sig;
    char signals[1024];
    ret = recv(m_pipefd[0], signals,sizeof(signals), 0);
    if(ret == -1) {
        return false;
    }
    else if(ret == 0) {
        return false;
    }
    else {
        for(int i = 0; i < ret; ++i) {
            switch(signals[i]) {
                case SIGALRM:
                    timeout = true;
                    break;
                case SIGTERM:
                    stop_server = true;
                    break;
            }
        }
    }
    return true;
}
void webserver::dealwithread(int sockfd) {
    util_timer *timer = users_timer[sockfd].timer;

    //reactor
    if(1 == m_actormodel) {
        if(timer) {
            adjust_timer(timer);
        }

        m_threadpool->append(users + sockfd, 0);

        while(true) {
            if(1 == users[sockfd].improv) {
                if(1 == users[sockfd].time_flag) {
                    deal_timer(timer, sockfd);
                    users[sockfd].time_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    }
    else {
        //proactor
        if(users[sockfd].read_once()) {
            LOG_INFO("deal with the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));

            m_threadpool->append(users + sockfd);

            if(timer) {
                adjust_timer(timer);
            }
        }
        else {
            deal_timer(timer, sockfd);
        }
    }
}

void webserver::dealwithwirte(int sockfd) {
    util_timer *timer = users_timer[sockfd].timer;
    //reactor
    if(1 == m_actormodel) {
        if(timer) {
            adjust_timer(timer);
        }
        m_threadpool->append(users + sockfd, 1);

        while(true) {
            if(1 == users[sockfd].improv) {     //线程已执行read or write
                if(1 == users[sockfd].time_flag) {
                    deal_timer(timer, sockfd);
                    users[sockfd].time_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    }
    else {
        if(users[sockfd].write()) {
            LOG_INFO("send data to the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));
            if(timer) {
                adjust_timer(timer);
            }
        }
        else {
            deal_timer(timer, sockfd);
            //timer->userdata->address
        }
    }
}

void webserver::event_loop() {
    bool timeout = false;
    bool stop_server = false;

    while(!stop_server) {
        int number = epoll_wait(m_epfd, event, MAX_EVENT, -1);
        if(number < 0 && errno != EINTR) {  //EINTR interrupted system call
            LOG_ERROR("%s", "epoll fail");
            break;
        }

        for(int i = 0; i < number; ++i) {
            int sockfd = event[i].data.fd;
            if(sockfd == m_listenfd) {
                LOG_INFO("webserver::event_loop_339:%d is listening", sockfd);
                bool flag = dealclientdata();
                if(false == flag) {
                    continue;
                }
            }
            else if(event[i].events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)){
                util_timer *timer = users_timer[sockfd].timer;
                deal_timer(timer, sockfd);
            }
            else if(sockfd == m_pipefd[0] && (event[i].events & EPOLLIN)) {
                bool flag = dealwithsignal(timeout, stop_server);
                if(false == flag) {
                    LOG_ERROR("%s", "dealwithsignal fail");
                }
            }
            else if(event[i].events & EPOLLIN) {
                dealwithread(sockfd);
            }
            else if(event[i].events & EPOLLOUT) {
                dealwithwirte(sockfd);
            }   

        }
        if(timeout) {
            utils.time_handler();
            LOG_INFO("%s", "timer tick");
            timeout = false;
        }
    }
}