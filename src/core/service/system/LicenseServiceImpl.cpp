#include "LicenseServiceImpl.h"
// ===================== 新增：Qt 线程 + 随机数头文件 =====================
#include <QThread>
#include <QRandomGenerator>

LicenseServiceImpl::LicenseServiceImpl(std::shared_ptr<ILicenseDao> dao)
    : m_dao(std::move(dao)) {
}

bool LicenseServiceImpl::saveLicenseCode(const QString& code) {
    if (code.isEmpty()) return false;
    return m_dao->writeLicenseCode(code);
}

QString LicenseServiceImpl::loadLicenseCode() const {
    return m_dao->readLicenseCode();
}

bool LicenseServiceImpl::isLicenseValid(const QString& code) const {
    // ===================== 核心：模拟2~4秒随机耗时验证 =====================
    // 生成 2000ms ~ 4000ms 随机等待时间
    int delayMs = QRandomGenerator::global()->bounded(2000, 4001);
    // 后台线程休眠（不阻塞UI主线程）
    QThread::msleep(delayMs);

    // 原有验证逻辑
    return code.startsWith("QT6-") && code.length() == 16;
}
