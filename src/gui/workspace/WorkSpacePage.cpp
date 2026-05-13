#include "WorkSpacePage.h"
#include "ui_WorkSpacePage.h"
#include "device/DevicePage.h"
#include "processing/ProcessingPage.h"
#include "realtime/RealtimePage.h"
#include "well/WellPage.h"
#include <QDebug>

WorkSpacePage::WorkSpacePage(QWidget* parent)
// 初始化基类：BaseDataOperateWidget
    : BaseDataOperateWidget(parent)
      , ui(new Ui::WorkSpacePage)
{
    ui->setupUi(this);

    QTabWidget* tabWidget = ui->tabWidget;
    // 移除临时页面
    if (tabWidget->count() > 0)
    {
        QWidget* tempPage = tabWidget->widget(0);
        tabWidget->removeTab(0);
        delete tempPage;
    }

    // 添加子页面 + 注册到基类子接口列表（关键：实现批量管理）
    addTabPage(new WellPage(this), "油井管理");
    //TODO 实时数据页面继承BaseDataOperateWidget
    addTabPage(new RealtimePage(this), "实时数据");
    addTabPage(new ProcessingPage(this), "数据处理");
    addTabPage(new DevicePage(this), "设备控制");
}

WorkSpacePage::~WorkSpacePage()
{
    delete ui;
}

// ==============================================
// 工具方法实现：统一封装标签页添加 + 子控件注册
// ==============================================
void WorkSpacePage::addTabPage(QWidget *page, const QString &tabName)
{
    if (!page || tabName.isEmpty()) {
        qDebug() << "标签页添加失败：参数无效（空指针/空名称）";
        return;
    }

    // 日志：开始添加标签页
    qDebug() << "开始添加标签页：" << tabName;
    ui->tabWidget->addTab(page, tabName);

    // 安全转换为接口类型
    IDataOperateInterface* dataInterface = dynamic_cast<IDataOperateInterface*>(page);
    if (dataInterface) {
        // 日志：转换&注册成功
        qDebug() << "接口转换成功，注册子控件：" << tabName;
        addChild(dataInterface);
    } else {
        // 日志：转换失败（警告）
        qWarning() << "接口转换失败！该页面未实现 IDataOperateInterface：" << tabName;
    }
}

// ==============================================
// 重写基类纯虚函数：当前WorkSpacePage无自身UI数据
// 后续如果需要清空/加载当前页面自身控件，在这里实现
// ==============================================
void WorkSpacePage::clearSelfDataAndDisableBtn()
{
    // 自身无数据，空实现
}

void WorkSpacePage::loadSelfDataAndEnableBtn()
{
    // 自身无数据，空实现
}
