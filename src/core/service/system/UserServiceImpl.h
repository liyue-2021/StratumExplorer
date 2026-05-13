#ifndef USERSERVICEIMPL_H
#define USERSERVICEIMPL_H

#include "service/system/IUserService.h"
#include "dao/system/IUserDao.h"
#include "models/po/IUser.h"

/**
 * @brief 用户服务实现类
 * 依赖：UserDao
 * 管理：CoreContext
 */
class UserServiceImpl : public IUserService
{
public:
    explicit UserServiceImpl(IUserDao* userDao);
    ~UserServiceImpl() override = default;

    // 禁用拷贝
    UserServiceImpl(const UserServiceImpl&) = delete;
    UserServiceImpl& operator=(const UserServiceImpl&) = delete;

    // 实现接口
    bool initUserTable() override;
    QList<IUser> findAllUsers() override;
    IUser findUserById(qint64 id) override;
    bool addUser(const IUser& user) override;
    bool batchAddUsers(const QList<IUser>& users) override;
    bool updateUser(const IUser& user) override;
    bool deleteUserById(qint64 id) override;

private:
    // 业务校验工具
    bool validateUser(const IUser& user);

private:
    IUserDao* m_userDao;
};

#endif // USERSERVICEIMPL_H
