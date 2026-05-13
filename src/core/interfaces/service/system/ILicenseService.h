#ifndef ILICENSESERVICE_H
#define ILICENSESERVICE_H

#include <QString>

class ILicenseService {
public:
    virtual ~ILicenseService() = default;

    virtual bool saveLicenseCode(const QString& code) = 0;
    virtual QString loadLicenseCode() const = 0;
    virtual bool isLicenseValid(const QString& code) const = 0; // 示例业务逻辑
};

#endif // ILICENSESERVICE_H