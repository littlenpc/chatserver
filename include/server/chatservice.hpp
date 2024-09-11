// 服务层代码
#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>

#include "json.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

// C++11新语法：using给已经存在的类型定义新的类型名称
// msgid对应的回调，表示处理消息的事件回调方法类型
// conn是连接，js就是发送的消息
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;

// 主要是做业务
// 聊天服务器业务类，单例模式
// 要给msg_id映射一个事件回调
class ChatService
{
public:
    // 获取单例对象的接口函数
    static ChatService *instance();
    // 因为下面几个业务都是网络层派发回来的回调，所以参数都是一致的
    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 一对一聊天业务
    void onechat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理注销业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    // 服务器异常，业务重置的方法
    void reset();
    // 从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int, string);

private:
    ChatService(); // 构造函数私有化

    // 存储消息id和对应的业务处理方法
    // 消息处理器表：存的是msg_id对应的处理操作
    // 这个表不需要考虑线程安全，因为在运行过程中不会添加、删除或修改业务，只是调用业务
    // key 应该就是msg_id
    unordered_map<int, MsgHandler> _msgHandlerMap;

    // 存储在线用户的通信连接
    // 这个map最后会被多个线程调用
    // 因为onMessage本身就会被多个线程调用，且不同用户可能在不同的工作线程中响应，进行一系列修改map的操作
    // 所以这个map还需要考虑线程安全问题
    // int 是用户的id
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // 定义互斥锁，保证_userConnMap的线程安全
    mutex _connMutex;

    // 数据操作类对象 --- user表
    // 服务只依赖于model类，不做具体的数据库相关操作
    // 数据库相关操作都封装在model中了，而且model给业务层提供的都是对象，而不是数据库的字段
    UserModel _userModel;
    // 数据操作类对象 --- offlinemessage表
    OfflineMsgModel _offlineMsgModel;
    // 数据操作类对象 --- friend表
    FriendModel _friendModel;
    // 数据操作类对象 --- groupuser表以及allgroup表
    GroupModel _groupModel;

    // redis操作对象
    Redis _redis;
};

#endif