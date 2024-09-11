#ifndef GROUP_H
#define GROUP_H

#include "groupuser.hpp"
#include <string>
#include <vector>
using namespace std;

// 匹配AllGroup表的ORM类
// ORM - object relation map - 对象关系映射
class Group
{
public:
    Group(int id = -1, string name = "", string desc = "")
    {
        this->id = id;
        this->name = name;
        this->desc = desc;
    }

    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setDesc(string desc) { this->desc = desc; }

    int getId() { return this->id; }
    string getName() { return this->name; }
    string getDesc() { return this->desc; }
    vector<GroupUser> &getUsers() { return this->users; }

private:
    // 群组id
    int id;
    // 群组名字
    string name;
    // 群组功能描述
    string desc;
    // 群组成员列表
    // 为什么不用User类，因为还需要给用户添加角色：管理员/普通用户
    vector<GroupUser> users;
};

#endif