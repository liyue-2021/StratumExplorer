// LicenseDaoImpl.h
#ifndef LICENSEDAOIMPL_H
#define LICENSEDAOIMPL_H

#include "dao/system/ILicenseDao.h"
#include <QSettings>

class LicenseDaoImpl : public ILicenseDao {
public:
    explicit LicenseDaoImpl(const QString& organization = "rlgd",
                            const QString& application = "StratumExplorer");
    ~LicenseDaoImpl() override = default;

    bool writeLicenseCode(const QString& code) override;
    QString readLicenseCode() const override;

private:
    QSettings m_settings;
};

#endif // LICENSEDAOIMPL_H
