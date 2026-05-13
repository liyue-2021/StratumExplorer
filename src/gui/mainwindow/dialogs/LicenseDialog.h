#ifndef LICENSEDIALOG_H
#define LICENSEDIALOG_H

// 替换：继承对话框专用基类（已封装 QDialog + IDataOperateInterface）
#include "widgets/BaseDataOperateDialog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class LicenseDialog; }
QT_END_NAMESPACE

// 核心修改：继承 BaseDataOperateDialog，无需重复继承 QDialog/接口
class LicenseDialog : public BaseDataOperateDialog
{
    Q_OBJECT

public:
    explicit LicenseDialog(QWidget *parent = nullptr);
    ~LicenseDialog();

    signals:
        void requestLoadLicense();
    void requestValidateSaveLicense(const QString& code);

private slots:
    void onLicenseLoaded(const QString& code);
    void onLicenseSaveResult(bool success);
    void onCloseButtonClicked();

private:
    void setUIBusy(bool isBusy);

    // 重写基类纯虚函数：自身数据处理逻辑
protected:
    void clearSelfDataAndDisableBtn() override;
    void loadSelfDataAndEnableBtn() override;

private:
    Ui::LicenseDialog *ui;
};

#endif // LICENSEDIALOG_H
