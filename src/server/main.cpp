#include "chatserver.hpp"
#include "chatservice.hpp"
#include <iostream>
#include <signal.h>
using namespace std;

// 处理服务器ctrl+c结束后，重置user的状态信息
void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0); // 表示程序正常结束执行，并返回退出码 0 给操作系统
}


int main(int argc, char **argv){

    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);
    
    // signal是一个带signum和handler两个参数的函数
    // 准备捕捉或屏蔽的信号由参数signum给出，接收到指定信号时将要调用的函数有handler给出
    signal(SIGINT, resetHandler);

    EventLoop loop;
    InetAddress addr(ip, port); // 固定要连接的IP地址和端口号
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop(); // epoll_wait以阻塞方式等待新用户连接，已连接用户的读写事件等

    return 0;
}