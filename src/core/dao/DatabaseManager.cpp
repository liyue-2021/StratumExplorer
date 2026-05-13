#include "dao/databasemanager.h"
//TODO 增加全局切换数据库的方法

DatabaseManager::DatabaseManager(const QString& defaultDbPath)
    : m_defaultDbPath(defaultDbPath)
{
    // 注册 SQLite 驱动（全局仅执行一次）
    if (!QSqlDatabase::drivers().contains("QSQLITE")) {
        m_lastError = "SQLite 驱动未找到！";
    }
}

DatabaseManager::~DatabaseManager()
{
    // 关闭所有数据库连接
    const QStringList connections = QSqlDatabase::connectionNames();
    for (const QString& name : connections) {
        if (name.startsWith("sqlite_thread_")) {
            QSqlDatabase::removeDatabase(name);
        }
    }
}

QString DatabaseManager::getConnectionName() const
{
    // 基于线程ID生成唯一连接名，保证线程隔离
    return QString("sqlite_thread_%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
}

bool DatabaseManager::openDatabase(const QString& connectionName, const QString& dbPath)
{
    // 如果连接已存在，先关闭
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase oldDb = QSqlDatabase::database(connectionName);
        if (oldDb.isOpen()) {
            oldDb.close();
        }
        QSqlDatabase::removeDatabase(connectionName);
    }

    // 创建新连接
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(dbPath);

    // 打开数据库
    if (!db.open()) {
        m_lastError = db.lastError().text();
        return false;
    }

    // SQLite 优化设置
    db.exec("PRAGMA journal_mode=WAL;");  // 写模式
    db.exec("PRAGMA synchronous=NORMAL;"); // 同步模式
    return true;
}

/**
 * TODO 修改为全局切换数据库而不是单独线程切换
 * @param dbPath
 * @return
 */
bool DatabaseManager::switchDatabase(const QString& dbPath)
{
    const QString connName = getConnectionName();
    return openDatabase(connName, dbPath);
}

QSqlDatabase DatabaseManager::getDatabase()
{
    const QString connName = getConnectionName();

    // 无连接 → 自动打开默认数据源
    if (!QSqlDatabase::contains(connName)) {
        openDatabase(connName, m_defaultDbPath);
    }

    return QSqlDatabase::database(connName);
}

void DatabaseManager::closeCurrentConnection()
{
    const QString connName = getConnectionName();
    if (QSqlDatabase::contains(connName)) {
        QSqlDatabase db = QSqlDatabase::database(connName);
        if (db.isOpen()) {
            db.close();
        }
        QSqlDatabase::removeDatabase(connName);
    }
}

// ------------------- 工具方法实现 -------------------
bool DatabaseManager::exec(const QString& sql)
{
    QSqlQuery query(getDatabase());
    if (!query.exec(sql)) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::exec(const QString& sql, const QMap<QString, QVariant>& params)
{
    QSqlQuery query(getDatabase());
    query.prepare(sql);
    for (auto it = params.begin(); it != params.end(); ++it) {
        query.bindValue(it.key(), it.value());
    }
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

QSqlQuery DatabaseManager::query(const QString& sql)
{
    QSqlQuery query(getDatabase());
    query.exec(sql);
    m_lastError = query.lastError().text();
    return query;
}

QSqlQuery DatabaseManager::query(const QString& sql, const QMap<QString, QVariant>& params)
{
    QSqlQuery query(getDatabase());
    query.prepare(sql);
    for (auto it = params.begin(); it != params.end(); ++it) {
        query.bindValue(it.key(), it.value());
    }
    query.exec();
    m_lastError = query.lastError().text();
    return query;
}

QString DatabaseManager::lastError() const
{
    return m_lastError;
}
