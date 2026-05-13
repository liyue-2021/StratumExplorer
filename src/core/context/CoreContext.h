#ifndef CORECONTEXT_H
#define CORECONTEXT_H

#include <memory>
#include <QString>
#include <QList>
#include <QThread>
// 基类头文件

// 引入接口头文件
#include "context/ICoreContext.h"
#include "dao/system/ILicenseDao.h"
#include "dao/DatabaseManager.h"
#include "dao/system/IUserDao.h"

// 实现类：公开继承纯虚接口
class CoreContext : public ICoreContext
{
public:
    // 禁止拷贝、移动、赋值
    CoreContext(const CoreContext&) = delete;
    CoreContext& operator=(const CoreContext&) = delete;
    CoreContext(CoreContext&&) = delete;
    CoreContext& operator=(CoreContext&&) = delete;

    friend class CoreContextFactory;

    // 1. 获取【软件全局配置】数据库（固定，不切换）
    std::shared_ptr<DatabaseManager> getConfigDatabaseManager() const;
    // 2. 获取【项目数据配置】数据库（可切换）
    std::shared_ptr<DatabaseManager> getProjectDatabaseManager() const;
    // 3. 动态切换【项目数据库】（打开新项目时调用）
    void switchProjectDatabase(const QString& projectDbPath);

    // ============= 服务接口获取 =============
    std::shared_ptr<ILicenseService> getLicenseService() const override;

protected:
    // ============= 内部DAO获取（接口不暴露） =============
    std::shared_ptr<ILicenseDao> getLicenseDao() const;
    std::shared_ptr<IUserDao> getUserDao() const;

private:
    CoreContext();  // 私有构造（单例）
    ~CoreContext();

    // 数据库管理
    std::shared_ptr<DatabaseManager> m_configDb;
    std::shared_ptr<DatabaseManager> m_projectDb;

    // 表初始化
    bool initConfigTables();
    bool initProjectTables();
    bool initAllTables();

    // 成员实例
    std::shared_ptr<ILicenseDao> m_licenseDao;
    std::shared_ptr<ILicenseService> m_licenseService;
    std::shared_ptr<IUserDao> m_userDao;

};

#endif // CORECONTEXT_H
