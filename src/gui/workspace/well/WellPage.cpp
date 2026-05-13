#include "WellPage.h"
#include "ui_WellPage.h"

// 构造函数：初始化父类 BaseDataOperateWidget
WellPage::WellPage(QWidget* parent) :
    BaseDataOperateWidget(parent), ui(new Ui::WellPage)
{
    ui->setupUi(this);
}

WellPage::~WellPage()
{
    delete ui;
}

// 重写基类纯虚函数：清空自身UI数据/禁用按钮
void WellPage::clearSelfDataAndDisableBtn()
{
    // ========================
    // 后续在这里写业务逻辑：
    // 1. 清空WellPage自己的输入框、表格、标签等
    // 2. 禁用页面操作按钮
    // ========================
}

// 重写基类纯虚函数：加载自身UI数据/启用按钮
void WellPage::loadSelfDataAndEnableBtn()
{
    // ========================
    // 后续在这里写业务逻辑：
    // 1. 加载WellPage自己的业务数据
    // 2. 启用页面操作按钮
    // ========================
}
