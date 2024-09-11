#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"

#include <functional>
#include <string>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;


/*
网络模块代码
使用muduo库得到了一个非常强大的基于事件驱动的I/O复用epoll+线程池的网络代码
是完全基于reactor模型的，设置了四个线程，有一个主reactor是I/O线程，三个子reactor是工作线程
主reactor主要负责新用户的链接
子reactor主要负责已连接用户的读写事件的处理
*/ 
// 初始化聊天服务器对象
ChatServer::ChatServer(EventLoop *loop,               // 事件循环
                       const InetAddress &listenAddr, // IP+Port --- IP地址+端口号
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 注册链接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    _server.setThreadNum(4);
}

// 启动服务
void ChatServer::start()
{
    _server.start();
}

// 上报连接相关信息的回调函数，即用户的连接和断开
void ChatServer::onConnection(const TcpConnectionPtr & conn)
{
    // 表示客户端断开连接
    if(!conn->connected())
    {
        // 当客户端异常关闭时的处理
        ChatService::instance()->clientCloseException(conn);
        // 关闭连接
        conn->shutdown();
    }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,  // 连接
                           Buffer *buffer,                // 缓冲区
                           Timestamp time)                // 接收到数据的时间信息
{
    // 从buffer的缓冲区拿到数据，放到buf中
    string buf = buffer->retrieveAllAsString();

    // 数据的反序列化，相当于对数据进行解码
    // 其中一定包含了message_id或者其他信息，以表示业务
    json js = json::parse(buf);

    // 目的：完全解耦网络模块的代码和业务模块的代码
    // 为了防止网络模块和业务模块耦合到一起
    // 通过js["msgid"]绑定一个回调操作，一个msgid对应一个操作
    // 这样就可以通过msgid获取一个业务处理器handler（事先绑定好的，就是构造函数中绑定的）
    // 通过回调handler，可以把接收到的数据，例如conn，js，time等等传进去

    // js["msgid"].get<int>() 将js中的msgid获取到，并转换为int型
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    // 回调信息绑定好的事件处理器，来执行相应的业务处理
    msgHandler(conn, js, time);
}
