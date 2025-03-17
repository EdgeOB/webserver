#ifndef SQL_CONNECTION_POOL_H
#define SQL_CONNECTION_POOL_H

#include <mysql/mysql.h>
#include <list>
#include <string>
#include "../lock/locker.h"

using namespace std;

class connection_pool{
public:
    MYSQL* get_connection();
    bool release_connection(MYSQL *conn);
    int getFreeconn();
    void destory_pool();
    static connection_pool* getInstance();
    void init(string url, string name, string passwd, string databases, int port, int maxconn, int close_log);
private:
    connection_pool();
    ~connection_pool();

    int m_maxconn;
    int m_freeconn;
    int m_curconn;
    locker m_mutex;
    list<MYSQL*> mysql_pool;
    sem reserve;

public:
    string m_url;
    string m_port;
    string username;
    string password;
    string database;
    int m_close_log;
};

class connectionRAII{
public:
    connectionRAII(MYSQL **mysql, connection_pool *connPool);
    ~connectionRAII();

private:
    MYSQL *connRAII;
    connection_pool *poolRAII;
};

#endif