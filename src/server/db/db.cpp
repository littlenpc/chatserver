#include "db.h"
#include <muduo/base/Logging.h>

// 数据库配置信息
// 数据库地址、用户名、密码、表名
static string server = "127.0.0.1";
static string user = "root";
static string password = "123456789";
static string dbname = "chat";

// 初始化数据库连接
MySQL::MySQL()
{
    // 这里不是连接初始化，只是开辟了一块存储连接数据的资源空间。
    _conn = mysql_init(nullptr);
}
// 释放数据库连接资源
MySQL::~MySQL()
{
    // 将构造函数中开辟的空间释放掉了
    if (_conn != nullptr)
        mysql_close(_conn);
}

// 连接数据库
bool MySQL::connect()
{
    // 参数分别是：存储连接数据的内存、server的IP地址、用户名、
    // 密码、所要连接的数据库、mysql server的默认端口号、最后两个不关注
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
                                  password.c_str(), dbname.c_str(), 3306, nullptr, 0);

    if (p != nullptr)
    {
        // 作用：使代码可以支持中文
        // C和C++代码默认的编码字符是ASCII，如果不设置，从mysql上拉下来的中文会显示为'?'
        // 向数据库对象发送 sql 指令
        mysql_query(_conn, "set names gbk");
        LOG_INFO << "connect mysql success!"; // 连接成功的日志
    }
    else
    {
        LOG_INFO << "connect mysql fail!";
    }
    return p;
}
// 更新操作
bool MySQL::update(string sql)
{
    // c_str()函数返回一个指向正规C字符串的指针常量, 内容与本string串相同。
    // int mysql_query(MYSQL *mysql, const char *q); --- 向数据库对象发送 sql 指令
    // mysql表示 MYSQL 对象；q 表示要执行的 sql 语句；返回值为 0 则表示 sql 执行成功，反之则表示执行失败。
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "更新失败!";
        return false;
    }
    return true;
}
// 查询操作
MYSQL_RES *MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str())) // 向数据库对象发送 sql 指令
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "查询失败!";
        return nullptr;
    }
    return mysql_use_result(_conn);
}

// 获取连接
MYSQL* MySQL::getConnection()
{
    return _conn;
}