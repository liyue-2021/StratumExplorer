#include "LicenseController.h"

// 🔥 构造函数同步修改：std::shared_ptr
LicenseController::LicenseController(std::shared_ptr<ILicenseService> service, QObject *parent)
    : ILicenseController(parent)
    , m_service(std::move(service))  // 逻辑不变
{

}

// ===================== 业务逻辑 完全不用改 =====================
void LicenseController::onRequestLoadLicense()
{
    QString code = m_service->loadLicenseCode();
    emit licenseLoaded(code);
}

void LicenseController::onRequestValidateSave(const QString& code)
{
    bool valid = m_service->isLicenseValid(code);
    if (!valid) {
        emit saveResult(false);
        return;
    }

    bool saveSuccess = m_service->saveLicenseCode(code);
    emit saveResult(saveSuccess);
}
