#include "config.h"

int main(int argc, char *argv[]) {
    // 数据库连接配置（默认值）
    string user = "root";
    string passwd = "123456";
    string databasename = "webserver";

    // 解析命令行参数（端口、日志等配置）
    config config;
    config.parse_arg(argc, argv);

    // 初始化Web服务器实例
    webserver server;
    server.init(config.port, user, passwd, databasename, config.log_write, 
                config.opt_linger, config.trigmode, config.sql_num, 
                config.thread_num, config.close_log, config.actor_model);
    
    // 初始化日志系统
    //log
    server.log_write();

    // 初始化数据库连接池
    server.sql_pool();

    // 创建线程池
    server.thread_pool();

    // 设置触发模式（LT/ET）
    server.trig_mode();

    // 开始监听网络端口
    server.event_listen();

    // 进入事件循环（主线程阻塞在此）
    server.event_loop();

    return 0;
}