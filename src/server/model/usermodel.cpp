#include "usermodel.hpp"
#include "db.h"
#include <iostream>
using namespace std;

// User表的增加方法
bool UserModel::insert(User &user)
{
    // 1 组装sql语句
    // sql里面的其实就是要执行的sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')",
            user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        // 将sql更新到mysql中
        if (mysql.update(sql))
        {
            // 获取插入成功的用户数据生成的主键id
            user.setId(mysql_insert_id(mysql.getConnection()));
            // 注册成功
            return true;
        }
    }
    // 否则注册失败
    return false;
}

// 根据用户id号码查询用户信息
User UserModel::query(int id)
{
    // 1 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        // 如果res不为空，代表查成功
        if (res != nullptr)
        {
            // 查到之后，查到的一行的信息用row保存
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);

                // 将上面开辟的res指针释放掉
                mysql_free_result(res);
                return user;
            }
        }
    }

    // 如果没有找到，则返回一个默认构造，id = -1
    return User();
}

// 更新用户的状态信息
bool UserModel::updateState(User user)
{
    // 1 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = %d", user.getState().c_str(), user.getId());

    MySQL mysql;
    if (mysql.connect())
    {
        // 将sql更新到mysql中
        if (mysql.update(sql))
        {
            return true;
        }
    }
    // 否则更新失败
    return false;
}

// 重置用户的状态信息
void UserModel::resetState()
{
    // 1 将所有状态为online的用户状态修改为offline
    char sql[1024] = "update user set state = 'offline' where state = 'online'";

    MySQL mysql;
    if (mysql.connect())
    {
        // 将sql更新到mysql中
        mysql.update(sql);
    }
}