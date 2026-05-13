#ifndef LICENSECONTROLLER_H
#define LICENSECONTROLLER_H

#include "controller/system/ILicenseController.h"
#include "service/system/ILicenseService.h"
#include <memory>  // 替换：std::shared_ptr 头文件
// 移除 QSharedPointer —— 这里不需要！

class LicenseController : public ILicenseController
{
    Q_OBJECT
public:
    // 🔥 修复：构造函数参数改为 std::shared_ptr（匹配 Service 类型）
    explicit LicenseController(std::shared_ptr<ILicenseService> service, QObject *parent = nullptr);

protected:
    void onRequestLoadLicense() override;
    void onRequestValidateSave(const QString& code) override;

private:
    // 🔥 修复：成员变量改为 std::shared_ptr
    std::shared_ptr<ILicenseService> m_service;
};

#endif // LICENSECONTROLLER_H
