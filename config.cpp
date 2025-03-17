#include "config.h"


config::config() {
    port = 9005;

    log_write = 1;

    trigmode = 0;

    listen_trigmode = 0;
    
    conn_trigmode = 0;

    opt_linger = 0;

    sql_num = 8;

    thread_num = 8;

    close_log = 0;
    //并发模式默认proactor
    actor_model = 0;
}

void config::parse_arg(int argc, char *argv[]) {
    int opt;
    const char *str = "p:l:m:o:s:t:c:a";

    while((opt = getopt(argc, argv, str)) != -1) {
        switch(opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'l':
                log_write = atoi(optarg);
                break;
            case 'm':
                trigmode = atoi(optarg);
                break;
            case 'o':
                opt_linger = atoi(optarg);
                break;
            case 's':
                sql_num = atoi(optarg);
                break;
            case 't':
                thread_num = atoi(optarg);
                break;
            case 'c':
                close_log = atoi(optarg);
                break;
            case 'a':
                actor_model = atoi(optarg);
                break;
            default:
                break;
        }
    }
}
