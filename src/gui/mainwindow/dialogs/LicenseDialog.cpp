#include "LicenseDialog.h"
#include "ui_LicenseDialog.h"
#include "context/GuiContextFactory.h"
#include "controller/system/ILicenseController.h"
#include <QMessageBox>
#include <QDebug>

LicenseDialog::LicenseDialog(QWidget *parent)
    // 初始化对话框基类
    : BaseDataOperateDialog(parent)
    , ui(new Ui::LicenseDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("授权管理"));

    auto controller = GuiContextFactory::getGuiContext().getLicenseController();
    if (!controller) {
        QMessageBox::critical(this, tr("错误"), tr("GUI 上下文未初始化，授权功能不可用！"));
        return;
    }

    // ===================== 按钮绑定 =====================
    // 加载注册码
    connect(ui->loadButton, &QPushButton::clicked, this, [this]() {
        // 1. 先禁用UI，防止重复点击
        setUIBusy(true);
        // 2. 发送请求到控制器
        emit requestLoadLicense();
    });

    // 验证并保存
    connect(ui->saveButton, &QPushButton::clicked, this, [this]() {
        QString code = ui->licenseEdit->text().trimmed();
        if (code.isEmpty()) {
            QMessageBox::warning(this, tr("警告"), tr("注册码不能为空！"));
            return;
        }
        // 1. 禁用UI
        setUIBusy(true);
        // 2. 发送请求
        emit requestValidateSaveLicense(code);
    });

    connect(ui->closeButton, &QPushButton::clicked, this, &LicenseDialog::onCloseButtonClicked);

    // ===================== 信号槽连接（跨线程自动适配） =====================
    connect(this, &LicenseDialog::requestLoadLicense,
            controller.get(), &ILicenseController::onRequestLoadLicense);

    connect(this, &LicenseDialog::requestValidateSaveLicense,
            controller.get(), &ILicenseController::onRequestValidateSave);

    connect(controller.get(), &ILicenseController::licenseLoaded,
            this, &LicenseDialog::onLicenseLoaded);

    connect(controller.get(), &ILicenseController::saveResult,
            this, &LicenseDialog::onLicenseSaveResult);

    //加载数据（调用基类规范方法）
    loadAllDataAndEnableBtn();
}

LicenseDialog::~LicenseDialog()
{
    delete ui;
}

// ===================== 核心优化：UI 繁忙状态管理 =====================
void LicenseDialog::setUIBusy(bool isBusy)
{
    // 禁用/启用 输入框
    ui->licenseEdit->setEnabled(!isBusy);
    // 禁用/启用 功能按钮
    ui->loadButton->setEnabled(!isBusy);
    ui->saveButton->setEnabled(!isBusy);
    // 关闭按钮可以保留启用（允许取消）

    if (isBusy) {
        // 繁忙时：显示等待提示
        ui->statusLabel->setText(tr("正在处理中，请稍候..."));
        ui->statusLabel->setStyleSheet("color: orange; font-weight: bold;");
    } else {
        // 空闲时：清空提示
        ui->statusLabel->clear();
        ui->statusLabel->setStyleSheet("");
    }
}

// ===================== 加载完成：恢复UI + 更新结果 =====================
void LicenseDialog::onLicenseLoaded(const QString& code)
{
    // 1. 恢复UI可用
    setUIBusy(false);
    // 2. 更新数据
    ui->licenseEdit->setText(code);
    // 3. 更新状态提示
    ui->statusLabel->setText(tr("注册码加载成功"));
    ui->statusLabel->setStyleSheet("color: blue; font-weight: bold;");
}

// ===================== 保存完成：恢复UI + 更新结果 =====================
void LicenseDialog::onLicenseSaveResult(bool success)
{
    // 1. 恢复UI可用
    setUIBusy(false);
    // 2. 更新状态提示
    if (success) {
        ui->statusLabel->setText(tr("注册码保存成功！"));
        ui->statusLabel->setStyleSheet("color: green; font-weight: bold;");
    } else {
        ui->statusLabel->setText(tr("注册码无效！格式错误"));
        ui->statusLabel->setStyleSheet("color: red; font-weight: bold;");
    }
}

void LicenseDialog::onCloseButtonClicked()
{
    accept();
}

// ===================== 重写基类纯虚函数 =====================
// 自身数据清空 + 控件禁用
void LicenseDialog::clearSelfDataAndDisableBtn()
{
    // 清空输入框数据
    ui->licenseEdit->clear();
    // 清空状态提示
    ui->statusLabel->clear();
    ui->statusLabel->setStyleSheet("");
    // 禁用所有功能按钮
    setUIBusy(true);
}

// 自身数据加载 + 控件启用
void LicenseDialog::loadSelfDataAndEnableBtn()
{
    // 启用所有功能按钮
    setUIBusy(false);
    // 主动触发加载授权码
    emit requestLoadLicense();
}
