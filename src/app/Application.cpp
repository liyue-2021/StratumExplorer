#include "application.h"
#include "MainWindow.h"
#include "context/ICoreContext.h"
#include "context/IGuiContext.h"
#include "context/CoreContextFactory.h"
#include "context/GuiContextFactory.h"
#include <QThreadPool>

Application::Application(QObject *parent)
    : QObject(parent), m_mainWindow(nullptr)
{
}

Application::~Application()
{
    delete m_mainWindow;
}

void Application::run()
{
    QThreadPool::globalInstance()->setObjectName("App_Global_Pool");

    // 🔥 核心修改：通过工厂类获取【接口实例】（不再直接依赖CoreContext实现类）
    ICoreContext &coreContext = CoreContextFactory::getCoreContext();

    // 1. 通过工厂获取 GUI 上下文（抽象接口）
    IGuiContext &guiContext = GuiContextFactory::getGuiContext();
    guiContext.init(coreContext);

    // 创建主窗口
    // 初始化GUI语言（界面翻译生效）
    m_mainWindow = new MainWindow();
    m_mainWindow->show();

    return;
}
