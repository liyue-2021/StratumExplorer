// LicenseDaoImpl.cpp
#include "LicenseDaoImpl.h"

LicenseDaoImpl::LicenseDaoImpl(const QString& organization, const QString& application)
    : m_settings(organization, application) {
}

bool LicenseDaoImpl::writeLicenseCode(const QString& code) {
    m_settings.setValue("License/Code", code);
    return true; // QSettings 一般不会失败，也可检查状态
}

QString LicenseDaoImpl::readLicenseCode() const {
    return m_settings.value("License/Code").toString();
}