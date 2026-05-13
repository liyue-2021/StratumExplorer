#include "GuiContext.h"
#include "controller/system/LicenseController.h"
#include "context/ICoreContext.h"

// 后台线程初始化
GuiContext::GuiContext()
{
    m_backgroundThread = new QThread();
    m_backgroundThread->setObjectName("GuiContext_Backend_Worker");
    m_backgroundThread->start();
}

// 安全释放线程
GuiContext::~GuiContext()
{
    if (m_backgroundThread) {
        m_backgroundThread->quit();
        m_backgroundThread->wait();
        delete m_backgroundThread;
        m_backgroundThread = nullptr;
    }
}

// 核心修改：参数类型同步为 ICoreContext&
void GuiContext::init(ICoreContext& coreContext)
{
    if (m_isInitialized) return;

    // 调用接口方法（无任何改动，多态自动匹配实现）
    auto licenseService = coreContext.getLicenseService();

    // 创建 Controller 逻辑不变
    m_licenseController = QSharedPointer<LicenseController>::create(licenseService, nullptr);

    // 移入后台线程
    m_licenseController->moveToThread(m_backgroundThread);

    m_isInitialized = true;
}

bool GuiContext::isInitialized() const
{
    return m_isInitialized;
}

QSharedPointer<ILicenseController> GuiContext::getLicenseController() const
{
    return m_isInitialized ? m_licenseController : nullptr;
}
