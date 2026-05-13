#include "MainWindow.h"
#include "ui_MainWindow.h"
// #include "MenuFactory.h"
#include "WelcomePage.h"
#include "workspace/WorkSpacePage.h"
#include "dialogs/LicenseDialog.h"

#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    // 初始化基类：BaseDataOperateMainWindow
    : BaseDataOperateMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // 🔥 一行代码加载所有菜单（工厂全权处理）
    // MenuFactory::instance()->loadMenusToBar(menuBar());

     statusBar()->showMessage("欢迎使用本软件！", 3000);

    // 移除占位菜单，仅在ui edit中查看用
    QMenu *tempMenu = ui->menuBar->findChild<QMenu*>("menuTemp");
    if (tempMenu) {
        ui->menuBar->removeAction(tempMenu->menuAction());
        delete tempMenu;   // 释放占位菜单内存
    }

    // 创建文件菜单（放在帮助菜单之前）
    QMenu *fileMenu = ui->menuBar->addMenu(tr("文件"));
    QAction *newProjectAction = fileMenu->addAction(tr("新建项目"));
    QAction *openProjectAction = fileMenu->addAction(tr("打开项目"));
    QAction *closeProjectAction = fileMenu->addAction(tr("关闭项目"));

    // 创建一级菜单 "帮助"
    QMenu *helpMenu = ui->menuBar->addMenu(tr( "帮助"));

    // 添加 "注册码" 菜单项
    QAction *licenseAction = helpMenu->addAction(tr("注册码"));
    connect(licenseAction, &QAction::triggered, this, [this]() {
        LicenseDialog dlg(this);
        dlg.exec();
    });

    // 获取 UI 中已经存在的 QStackedWidget
    QStackedWidget *stackedWidget = ui->stackedWidget;

    // 创建欢迎页面实例，并添加到堆栈中
    welcomePage = new WelcomePage(this);
    stackedWidget->addWidget(welcomePage);
    // 关键：注册子页面到基类，实现批量数据管理
    addChild(welcomePage);

    // 创建工作区页面实例，并添加到堆栈中
    workspacePage = new WorkSpacePage(this);
    stackedWidget->addWidget(workspacePage);
    // 关键：注册子页面到基类，实现批量数据管理
    addChild(workspacePage);

    // 设置初始显示页面（比如欢迎页）
    stackedWidget->setCurrentWidget(workspacePage);

    //强制增加布局，使子widget可以放大缩小
    QVBoxLayout *centralLayout = new QVBoxLayout(ui->centralwidget);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(ui->stackedWidget);

    //  添加临时切换菜单项
    QMenu *viewMenu = menuBar()->addMenu("仅开发使用！");   // 或者直接添加到现有菜单
    // 1. 切换页面
    QAction *toggleAction = viewMenu->addAction("切换页面");
    connect(toggleAction, &QAction::triggered, this, &MainWindow::togglePages);
    // 分隔线（区分功能，更清晰）
    viewMenu->addSeparator();
    // 2. 清除所有数据（调用基类批量清空：自身+所有子页面）
    QAction *clearDataAction = viewMenu->addAction("清除页面所有数据");
    connect(clearDataAction, &QAction::triggered, this, &BaseDataOperateMainWindow::clearAllDataAndDisableBtn);
    // 3. 加载所有数据（调用基类批量加载：自身+所有子页面）
    QAction *loadDataAction = viewMenu->addAction("加载页面所有数据");
    connect(loadDataAction, &QAction::triggered, this, &BaseDataOperateMainWindow::loadAllDataAndEnableBtn);
}

// ===================== 封装的工具方法：统一添加堆栈页面 =====================
void MainWindow::addStackedPage(QWidget *page)
{
    if (!page) {
        qDebug() << "堆栈页面添加失败：空指针";
        return;
    }

    // 添加到堆栈控件
    ui->stackedWidget->addWidget(page);
    qDebug() << "添加堆栈页面：" << page->metaObject()->className();

    // 安全转换为数据接口并注册
    IDataOperateInterface* dataInterface = dynamic_cast<IDataOperateInterface*>(page);
    if (dataInterface) {
        addChild(dataInterface);
        qDebug() << "子接口注册成功：" << page->metaObject()->className();
    } else {
        qWarning() << "接口转换失败：" << page->metaObject()->className();
    }
}


void MainWindow::togglePages()
{
    QStackedWidget *stackedWidget = ui->stackedWidget;
    if (stackedWidget->currentWidget() == welcomePage) {
        stackedWidget->setCurrentWidget(workspacePage);
    } else {
        stackedWidget->setCurrentWidget(welcomePage);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ===================== 重写基类纯虚函数 =====================
// 新增：接口函数空实现（统一规范，后续填充业务逻辑）
void MainWindow::clearSelfDataAndDisableBtn()
{

}

void MainWindow::loadSelfDataAndEnableBtn()
{

}
