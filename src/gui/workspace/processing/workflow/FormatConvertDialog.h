#ifndef PROCESSING_GUI_FORMAT_CONVERT_DIALOG_H
#define PROCESSING_GUI_FORMAT_CONVERT_DIALOG_H

#include <QDialog>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
namespace Ui
{
class FormatConvertDialog;
}
QT_END_NAMESPACE

class QTimer;

namespace processing::gui
{

/**
 * @brief 「数据格式转换」节点配置弹窗
 *
 * 界面布局见 FormatConvertDialog.ui（Qt Creator 中编辑）。
 * 后续节点弹窗建议同样：*.ui + 本类只写校验/信号/业务逻辑。
 */
class FormatConvertDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FormatConvertDialog(QWidget *parent = nullptr);
    ~FormatConvertDialog() override;

    void setParams(const QVariantMap &params);
    QVariantMap params() const;

signals:
    void paramsCommitted(const QVariantMap &params);
    void conversionSucceeded(const QVariantMap &params);
    void conversionFailed(const QString &message);

private slots:
    void onPickImport();
    void onPickOutput();
    void onStartConvert();
    void onSimulateProgress();

private:
    void setupConnections();
    bool validateInputs(QString *errorMessage) const;
    void setConverting(bool converting);
    void setStatus(const QString &text, bool isError = false);

    Ui::FormatConvertDialog *ui = nullptr;
    QTimer *m_simTimer = nullptr;
    int m_simProgress = 0;
    bool m_converting = false;
};

} // namespace processing::gui

#endif
