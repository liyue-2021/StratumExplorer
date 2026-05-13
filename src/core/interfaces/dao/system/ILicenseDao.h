#ifndef ILICENSEDAO_H
#define ILICENSEDAO_H

#include <QString>

class ILicenseDao {
public:
    virtual ~ILicenseDao() = default;

    virtual bool writeLicenseCode(const QString& code) = 0;
    virtual QString readLicenseCode() const = 0;
};

#endif // ILICENSEDAO_H