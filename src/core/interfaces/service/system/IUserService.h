#ifndef IUSERSERVICE_H
#define IUSERSERVICE_H

#include <QList>
#include "../models/po/IUser.h"

/**
 * @brief 用户业务服务接口
 * 核心：封装业务逻辑、事务、数据校验
 */
class IUserService
{
public:
    virtual ~IUserService() = default;

    // 初始化表
    virtual bool initUserTable() = 0;

    // 查询所有用户
    virtual QList<IUser> findAllUsers() = 0;

    // 根据ID查询
    virtual IUser findUserById(qint64 id) = 0;

    // 添加用户（带业务校验）
    virtual bool addUser(const IUser& user) = 0;

    // 批量添加用户
    virtual bool batchAddUsers(const QList<IUser>& users) = 0;

    // 更新用户
    virtual bool updateUser(const IUser& user) = 0;

    // 删除用户
    virtual bool deleteUserById(qint64 id) = 0;
};

#endif // IUSERSERVICE_H
