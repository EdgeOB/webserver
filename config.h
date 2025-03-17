#ifndef CONFIG_H
#define CONFIG_H
#include "webserver.h"

class config{
public:
    config();
    ~config() {}
    void parse_arg(int argc, char *argv[]);

    int port;
    //日志写入方式，默认0为同步
    int log_write;

    int trigmode;

    int listen_trigmode;

    int conn_trigmode;

    int opt_linger;

    int sql_num;

    int thread_num;

    int close_log;

    int actor_model;
};
#endif