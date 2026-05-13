#ifndef USERDAOIMPL_H
#define USERDAOIMPL_H

#include "dao/BaseDAO.h"
#include "dao/system/IUserDao.h"
#include "models/po/IUser.h"

/**
 * @brief 用户DAO实现类
 * 绑定：全局配置数据库 CoreContext::getConfigDatabaseManager()
 */
class UserDaoImpl : public BaseDAO, public IUserDao
{
public:
    // 构造函数：传入数据库管理器
    explicit UserDaoImpl(DatabaseManager* manager);
    ~UserDaoImpl() override = default;

    // 禁用拷贝
    UserDaoImpl(const UserDaoImpl&) = delete;
    UserDaoImpl& operator=(const UserDaoImpl&) = delete;

    // ===================== 实现接口 =====================
    bool initTable() override;
    QList<IUser> findAll() override;
    IUser findById(qint64 id) override;
    bool addUser(const IUser& user) override;
    bool batchAddUsers(const QList<IUser>& users) override;
    bool updateUser(const IUser& user) override;
    bool deleteById(qint64 id) override;

private:
    // ===================== 私有工具 =====================
    // 结果集映射：将数据库记录转为IUser对象
    void userMapper(QSqlQuery& query, IUser& user);
};

#endif // USERDAOIMPL_H
