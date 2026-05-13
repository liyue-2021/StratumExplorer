// CoreContext.cpp
#include "CoreContext.h"
#include "dao/system/LicenseDaoImpl.h"
#include "service/system/LicenseServiceImpl.h"
#include "dao/system/UserDaoImpl.h"
// #include "manager/DasDeviceManager.h"
// #include "manager/DssDeviceManager.h"
// #include "manager/DtsDeviceManager.h"
// #include "service/data/DataService.h"

CoreContext::CoreContext() {
    // 1. 初始化【软件全局配置库】（固定路径，文件持久化）
    m_configDb = std::make_shared<DatabaseManager>("StratumExplorer.db");
    // 2. 初始化【默认项目配置库】（后续可切换）
    m_projectDb = std::make_shared<DatabaseManager>("file::memory:?cache=default_project");

    m_licenseDao = std::make_shared<LicenseDaoImpl>("rlgd", "StratumExplorer");
    m_licenseService = std::make_shared<LicenseServiceImpl>(m_licenseDao);

    m_userDao = std::make_shared<UserDaoImpl>(m_configDb.get());

    // 初始化所有表
    initAllTables();

}

// 在析构函数中安全停止并退出线程
CoreContext::~CoreContext() {
}

bool CoreContext::initAllTables() {
    const bool configOk = initConfigTables();
    const bool projectOk = initProjectTables();
    return configOk && projectOk;
}

// 获取软件配置数据库
std::shared_ptr<DatabaseManager> CoreContext::getConfigDatabaseManager() const {
    return m_configDb;
}

// 获取项目配置数据库
std::shared_ptr<DatabaseManager> CoreContext::getProjectDatabaseManager() const {
    return m_projectDb;
}

// 动态切换项目数据库（打开新项目时调用）
void CoreContext::switchProjectDatabase(const QString& projectDbPath) {
    m_projectDb->switchDatabase(projectDbPath);
    initProjectTables();
}

std::shared_ptr<ILicenseDao> CoreContext::getLicenseDao() const {
    return m_licenseDao;
}

std::shared_ptr<ILicenseService> CoreContext::getLicenseService() const {
    return m_licenseService;
}

// 新增：获取UserDao
std::shared_ptr<IUserDao> CoreContext::getUserDao() const {
    return m_userDao;
}

// ===================== 建表方法实现 =====================

bool CoreContext::initConfigTables() {
    bool success = true;
    // 初始化全局配置库的所有表（按需添加）
    return success;
}

bool CoreContext::initProjectTables() {
    bool success = true;
    // 初始化项目数据库的所有表
    success &= m_userDao->initTable();
    return success;
}
