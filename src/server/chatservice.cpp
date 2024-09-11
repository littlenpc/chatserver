#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <vector>
using namespace std;
using namespace muduo;

// 获取单例对象的接口函数
ChatService *ChatService::instance()
{
    // 单例对象
    static ChatService service;
    return &service;
}

// 注册消息以及对应的handler回调操作，包括初始化成员变量和方法
ChatService::ChatService()
{
    // 业务设计核心，同时也是将网络模块和业务模块解耦的核心
    // 需要进行绑定，否则无法派发出去
    // LOGIN_MSG 对应的就是登录业务
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    // LOGINOUT_MSG 对应的就是注销业务
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});
    // REG_MSG 对应的就是注册业务
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    // ONE_CHAT_MSG 对应的就是一对一聊天业务
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::onechat, this, _1, _2, _3)});
    // ADD_FRIEND_MSG 对应的是添加好友业务
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    // 连接redis服务器
    if(_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

// 服务器异常，业务重置的方法
void ChatService::reset()
{
    // 把online状态的用户设置为offline
    _userModel.resetState();
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
// msgid 就是不同业务的编号，可以通过哈希表_msgHandlerMap查找到对应的业务
{
    // 记录错误日志，msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end()) // 表示没有找到
    {
        // 返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp)
        {
            // 通过muduo库日志进行错误信息打印
            // LOG_ERROR 表示错误类消息
            LOG_ERROR << "msgid: " << msgid << " can not find handler!"; // 表示找不到相应的处理器
        };
    }
    else
    {
        // 如果找到了，就返回对应的处理器
        return _msgHandlerMap[msgid];
    }
}

// 处理登录业务
// 即输入id + pwd 并检测是否对应正确，即可登录
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // js["id"]默认是字符串类型，.get<int>() 将其转换为int型
    int id = js["id"].get<int>();
    string pwd = js["password"];

    // 根据用户id号码查询用户信息
    User user = _userModel.query(id);
    // 查询到的user的id等于js["id"]并且密码正确，才能登录成功
    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            // 该用户已经登录，不允许重复登录
            // response - 响应
            json response;
            // LOGIN_MSG_ACK --- 对应的就是登录响应消息
            response["msgid"] = LOGIN_MSG_ACK;
            // errno = 2，表示响应出错，就会有errmsg说明错误信息
            response["errno"] = 2;
            response["errmsg"] = "this account is using, input another!"; // 该账号已经登录，请重新输入新账号
            // 登陆失败，将json发送回去
            // json.dump() -- 将json对象序列化为字符串格式
            conn->send(response.dump());
        }
        else
        {
            // 登录成功，记录用户连接信息
            // 添加互斥锁，保证线程安全 （而对于数据库中的并发操作不需要考虑，因为mysql server会保证多线程安全）
            // 互斥锁是在{}结束时，自动解锁，所以这里将两行代码放到{}中，以及时解锁，防止多线程并发程序变为单线程
            // 使锁的力度尽可能小
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            } // 出做用户，即{}结束自动解锁

            // id用户登录成功后，向redis订阅channel(id)
            // 也就是用户登录成功后，向详细队列中订阅一下，当有发给该用户的消息的时候，可以从消息队列中转发过来
            _redis.subscribe(id);

            // 登录成功，更新用户状态信息state: offline -> online
            // 先将user的状态改为online
            user.setState("online");
            // 再同步到user表中
            _userModel.updateState(user);

            // 准备给客户端返回消息
            // response - 响应
            json response;
            // LOGIN_MSG_ACK --- 对应的就是登录响应消息
            response["msgid"] = LOGIN_MSG_ACK;
            // 如果errno = 0，表示响应成功，没有出错
            // 如果errno = 1，表示响应出错，就会有errmsg说明错误信息
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询该用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            // vec不为空，表示有离线消息
            if(!vec.empty())
            {
                // 将vec添加到json中
                response["offlinemsg"] = vec;
                // 读取该用户的离线消息后，把该用户的所有离线消息删除掉
                _offlineMsgModel.remove(id);
            }

            // 查询该用户的好友信息，并返回
            vector<User> userVec = _friendModel.query(id);
            if(!userVec.empty())
            {
                vector<string> vec2;
                for(User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.emplace_back(js.dump());
                }
                response["friends"] = vec2;
            }

            // 查询用户的群组信息
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty())
            {
                // group:[{groupid:[xxx, xxx, xxx, xxx]}]
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }

                response["groups"] = groupV;
            }

            // 登录成功，将json发送回去
            // json.dump() -- 将json对象序列化为字符串格式
            conn->send(response.dump());
        }
    }
    else
    {
        // 该用户不存在或用户存在但密码错误，登录失败
        // response - 响应
        json response;
        // LOGIN_MSG_ACK --- 对应的就是登录响应消息
        response["msgid"] = LOGIN_MSG_ACK;
        // errno = 1，表示响应出错，就会有errmsg说明错误信息
        response["errno"] = 1;
        response["errmsg"] = "id or password is invalid!"; // 用户名或密码错误
        // 登陆失败，将json发送回去
        // json.dump() -- 将json对象序列化为字符串格式
        conn->send(response.dump());
    }
}

// 处理注册业务
// user表中一共有四个字段，填一个name password就可以
// id是注册成功之后返回给用户的，state也不用填
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);
    if (state)
    {
        // 注册成功
        // response - 响应
        json response;
        // REG_MSG_ACK --- 对应的就是注册响应消息
        response["msgid"] = REG_MSG_ACK;
        // 如果errno = 0，表示响应成功，没有出错
        // 如果errno = 1，表示响应出错，就会有errmsg说明错误信息
        response["errno"] = 0;
        response["id"] = user.getId();
        // 注册成功，将json发送回去
        // json.dump() -- 将json对象序列化为字符串格式
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        // response - 响应
        json response;
        // REG_MSG_ACK --- 对应的就是注册响应消息
        response["msgid"] = REG_MSG_ACK;
        // errno = 1，表示响应出错，可能需要error_message说明错误信息
        response["errno"] = 1;
        // 将json发送回去
        conn->send(response.dump());
    }
}


// 处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    // 添加互斥锁，保证线程安全 （而对于数据库中的并发操作不需要考虑，因为mysql server会保证多线程安全）
    // 互斥锁是在{}结束时，自动解锁，所以这里将两行代码放到{}中，以及时解锁，防止多线程并发程序变为单线程
    // 使锁的力度尽可能小
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }

    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(userid);

    // 更新用户的状态信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);

}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    // 通过conn去_userConnMap中查，找到响应的用户id
    // 1. 将找到的pair从_userConnMap中删除
    // 2. 修改数据库中该用户的状态，从online -> offline

    // 保存用户id，用来后面修改用户状态信息
    User user;
    // 添加互斥锁，保证线程安全 （而对于数据库中的并发操作不需要考虑，因为mysql server会保证多线程安全）
    // 互斥锁是在{}结束时，自动解锁，所以这里将两行代码放到{}中，以及时解锁，防止多线程并发程序变为单线程
    // 使锁的力度尽可能小
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // 保存用户id，用来后面修改用户状态信息
                user.setId(it->first);
                // 从_userConnMap中删除用户的链接信息
                _userConnMap.erase(it);
                break;
            }
        }
    } // {}结束自动解锁

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getId());

    // 更新用户的状态信息
    if (user.getId() != -1) // 防止没有找到
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 一对一聊天业务
void ChatService::onechat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 要发送的用户id
    int toid = js["toid"].get<int>();

    // 第一种情况，用户id和要发送给的用户toid在同一服务器上登录，可以直接转发
    // 因为要对_userConnMap进行操作，所以添加互斥锁，保证线程安全
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // toid在线，转发消息  服务器主动推送消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }

    // 第二种情况，用户id和要发送给的用户toid不在同一服务器上登录
    // 查询toid是否在线
    User user = _userModel.query(toid);
    if(user.getState() == "online")
    {
        // 向redis指定的通道channel发布消息
        _redis.publish(toid, js.dump());
        return;
    }

    // 第三种情况，表示toid不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}


// 添加好友业务
// msgid 对应添加好友，  id 用户id，  friendid 想添加的好友id
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _friendModel.insert(userid, friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    // _groupModel.createGroup(group) --- 将新建的群组创建并将群组信息保存到AllGroup表中
    if (_groupModel.createGroup(group))
    {
        // 存储群组创建人信息，存储到groupUser表中
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    // 存储到groupUser表中
    _groupModel.addGroup(userid, groupid, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    // 查询该群组中除了发消息的用户id之外，其他所有用户的id，方便后续消息转发
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    // 加锁
    lock_guard<mutex> lock(_connMutex);
    // 遍历所有用户，在线的直接转发，不在线的存储离线消息
    for (int id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 第一种情况：用户id和要发送给的用户toid在同一服务器上登录，可以直接转发
            // 转发群消息
            it->second->send(js.dump());
        }
        else
        {
            // 查询toid是否在线
            User user = _userModel.query(id);
            if(user.getState() == "online")
            {
                // 第二种情况：用户id和要发送给的用户toid不在同一服务器上登录，需要先向redis消息队列发布消息
                // 向redis指定的通道channel发布消息
                _redis.publish(id, js.dump());
            }
            else
            {
                // 第三种情况：用户toid离线
                // 存储离线群消息
                _offlineMsgModel.insert(id, js.dump());
            }
            
        }
    }
}

// 从redis消息队列中获取订阅的消息
// int userid --- 即时用户id，也是通道号，string msg --- 上报的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    // 加锁
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    // 如果在上报转发的过程中，toid用户下线了，则存储该用户的离线消息
    _offlineMsgModel.insert(userid, msg);
}