#include "sql_connection_pool.h"
#include "../log/log.h"
using namespace std;

connection_pool::connection_pool() {
    m_freeconn = 0;
    m_curconn = 0;
}
connection_pool::~connection_pool() {
    destory_pool();
}
connection_pool *connection_pool::getInstance() {
    static connection_pool connInstance;
    return &connInstance;
}

void connection_pool::init(string url, string name, string passwd, string databases, int port, int maxconn, int close_log) {
    m_url = url;
    username = name;
    password = passwd;
    database = databases;
    m_maxconn = maxconn;
    m_close_log = close_log;
    m_port = port;

    for(int i = 0; i < m_maxconn; ++i) {
        MYSQL *conn = NULL;
        conn = mysql_init(conn);

        if(conn == NULL) {
            LOG_ERROR("MYSQL error");
            exit(1);
        }

        conn = mysql_real_connect(conn, m_url.c_str(), username.c_str(), password.c_str(), database.c_str(),port, NULL, 0);
        
        if(conn == NULL) {
            LOG_ERROR("MYSQL error");
            exit(1);
        }

        mysql_pool.push_back(conn);
        ++m_freeconn;
    }

    reserve = sem(m_freeconn);

    m_maxconn = m_freeconn;
}

MYSQL* connection_pool::get_connection() {
    MYSQL *conn = NULL;

    if(0 == mysql_pool.size()) {
        return NULL;
    }
    reserve.wait();
    m_mutex.lock();

    conn = mysql_pool.front();
    mysql_pool.pop_front();
    ++m_curconn;
    --m_freeconn;

    m_mutex.unlock();
    return conn;
}
bool connection_pool::release_connection(MYSQL *conn) {
    if(conn == NULL) {
        LOG_INFO("conn is null");
        return false;
    }
    
    m_mutex.lock();

    mysql_pool.push_back(conn);
    // conn = NULL;
    ++m_freeconn;
    --m_curconn;

    m_mutex.unlock();
    reserve.post();
    return true;
}

int connection_pool::getFreeconn() {
    return m_freeconn;
}
//关闭连接
void connection_pool::destory_pool() {
    m_mutex.lock();
    if(mysql_pool.size()) {
        for(auto it = mysql_pool.begin(); it != mysql_pool.end(); ++it) {
            mysql_close(*it);
        }        
        m_freeconn = 0;
        m_curconn = 0;
        mysql_pool.clear();
    }
    m_mutex.unlock();
}

connectionRAII::connectionRAII(MYSQL **mysql, connection_pool *connPool) {
    *mysql = connPool->get_connection();
    connRAII = *mysql;
    poolRAII = connPool;
}

connectionRAII::~connectionRAII() {
    poolRAII->release_connection(connRAII);
}
