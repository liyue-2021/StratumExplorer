// LicenseServiceImpl.h
#ifndef LICENSESERVICEIMPL_H
#define LICENSESERVICEIMPL_H

#include "service/system/ILicenseService.h"
#include "dao/system/ILicenseDao.h"
#include <memory>

class LicenseServiceImpl : public ILicenseService {
public:
    explicit LicenseServiceImpl(std::shared_ptr<ILicenseDao> dao);
    ~LicenseServiceImpl() override = default;

    bool saveLicenseCode(const QString& code) override;
    QString loadLicenseCode() const override;
    bool isLicenseValid(const QString& code) const override;

private:
    std::shared_ptr<ILicenseDao> m_dao;
};

#endif // LICENSESERVICEIMPL_H