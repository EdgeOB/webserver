#ifndef HTTP_CONN_H
#define HTTP_CONN_H
#include<string>
#include<map>
#include<fcntl.h>
#include<sys/epoll.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<stdarg.h>
#include<sys/stat.h>
#include<sys/mman.h>
#include<sys/uio.h>
#include"../CGImysql/sql_connection_pool.h"
#include"../log/log.h"
using namespace std;
class http_conn{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFF_SIZE = 2048;
    static const int WRITE_BUFF_SIZE = 1024;
    //http请求方法
    enum METHOD{
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    //读取http请求的状态
    enum CHECK_STATE{
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEAD,
        CHECK_STATE_CONTENT
    };
    //服务器请求可能产生的结果
    enum HTTP_CODE{ 
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };

    enum LINE_STATUS{
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    void init(int sockfd, const sockaddr_in &addr, char*, int, int, string user, string password, string sqlname);
    void close_conn(bool read_close = true);
    void process();
    bool read_once();
    bool write();
    sockaddr_in *get_address() {
        return &m_address;
    }
    void initmysql_result(connection_pool *conn_pool);
    int time_flag;
    int improv;
private:
    void init();
    HTTP_CODE process_read();
    bool process_write(HTTP_CODE ret);
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_head(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();
    char *get_line(){ return m_read_buf + m_start_line; }
    LINE_STATUS parse_line();

    void unmap();
    bool add_response(const char *format,...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_len);
    bool add_content_type();
    bool add_content_length(int content_len);
    bool add_linger();
    bool add_blank_line();
public:
    static int m_epfd;
    static int m_user_count;
    MYSQL *mysql;
    int m_state; //0 read, 1 write
private:
    int m_sockfd;
    sockaddr_in m_address;
    char m_read_buf[READ_BUFF_SIZE];
    long m_read_idx;    //?为什么不用int

    long m_check_idx;   //有什么用
    int m_start_line;

    char m_write_buf[WRITE_BUFF_SIZE];
    int m_write_idx;

    CHECK_STATE m_check_state;
    METHOD m_method;

    char m_real_file[FILENAME_LEN];
    //从http请求行，请求头获取信息
    char *m_url;
    char *m_version;
    char *m_host;
    long m_content_length;  //记录content长度
    bool m_linger;  //connection:keep-alive
    
    char *m_file_address;
    struct stat m_file_stat;
    struct iovec m_iv[2];
    int m_iv_count;

    int cgi;
    char *m_string; //获取账号密码
    int bytes_to_send;
    int bytes_have_save;
    char *doc_root;

    map<string, string> user;
    int m_TRIGMode;
    int m_close_log;

    char sql_user[100];
    char sql_name[100];
    char sql_password[100];

};
#endif