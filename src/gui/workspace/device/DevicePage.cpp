#include "DevicePage.h"
#include "ui_DevicePage.h"

DevicePage::DevicePage(QWidget *parent)
    // 初始化基类
    : BaseDataOperateWidget(parent)
    , ui(new Ui::DevicePage)
{
    ui->setupUi(this);
}

DevicePage::~DevicePage()
{
    delete ui;
}

// 重写基类纯虚函数：清空当前页面自身数据/禁用控件
void DevicePage::clearSelfDataAndDisableBtn()
{
    // 后续在此处实现业务逻辑：清空输入框、禁用按钮等
}

// 重写基类纯虚函数：加载当前页面自身数据/启用控件
void DevicePage::loadSelfDataAndEnableBtn()
{
    // 后续在此处实现业务逻辑：加载数据、启用按钮等
}
