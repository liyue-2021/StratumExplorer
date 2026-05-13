#include "WelcomePage.h"
#include "ui_WelcomePage.h"
#include <QDateTime>

WelcomePage::WelcomePage(QWidget *parent)
    // 初始化基类
    : BaseDataOperateWidget(parent)
    , ui(new Ui::WelcomePage)
{
    ui->setupUi(this);
}

WelcomePage::~WelcomePage()
{
    delete ui;
}

// 重写基类纯虚函数：清空当前页面自身数据
void WelcomePage::clearSelfDataAndDisableBtn()
{
    // 核心：清空文本编辑框，模拟【清除数据】效果
    ui->textEdit->clear();
    // 可选：添加提示文字，告知用户已清空
    ui->textEdit->append("=== 已清空所有数据 ===");
}

// 重写基类纯虚函数：加载当前页面自身数据
void WelcomePage::loadSelfDataAndEnableBtn()
{
    // 核心：模拟加载业务数据，给文本框填充演示内容
    ui->textEdit->clear(); // 先清空原有内容
    // 模拟加载系统数据、欢迎信息、统计数据等
    // 1. 获取当前系统时间，格式化显示
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    // 2. 拼接HTML，插入动态系统时间
    QString html = QString(R"(
        <h2 style='color: #2E86AB;'>🎉 欢迎使用数据管理系统</h2>
        <p>----------------------------------------</p>
        <p><b>模拟加载数据完成！</b></p>
        <p>• 当前用户：管理员</p>
        <p>• 系统状态：正常运行</p>
        <p>• 数据版本：V1.0.0</p>
        <p>• 加载时间：%1</p>
        <p>----------------------------------------</p>
    )").arg(currentTime);
    // 设置到文本框
    ui->textEdit->setHtml(html);
}
