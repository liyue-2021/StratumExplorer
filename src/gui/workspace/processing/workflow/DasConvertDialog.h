#ifndef PROCESSING_GUI_DAS_CONVERT_DIALOG_H
#define PROCESSING_GUI_DAS_CONVERT_DIALOG_H

#include <QDialog>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
namespace Ui
{
class DasConvertDialog;
}
QT_END_NAMESPACE

class QTimer;
class QComboBox;

namespace processing::gui
{

/** @brief 「DAS数据转换」节点配置弹窗（界面见 DasConvertDialog.ui） */
class DasConvertDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DasConvertDialog(QWidget *parent = nullptr);
    ~DasConvertDialog() override;

    void setParams(const QVariantMap &params);
    QVariantMap params() const;

signals:
    void paramsCommitted(const QVariantMap &params);
    void transformSucceeded(const QVariantMap &params);
    void transformFailed(const QString &message);

private slots:
    void onPickInput();
    void onPickOutput();
    void onNormalizationChanged(int index);
    void onTransform();
    void onSimulateProgress();

private:
    void setupConnections();
    void setComboByText(QComboBox *combo, const QString &text) const;
    void updateNormalizationEnabled();
    bool validateInputs(QString *errorMessage) const;
    void setTransforming(bool transforming);
    void setStatus(const QString &text, bool isError = false);

    Ui::DasConvertDialog *ui = nullptr;
    QTimer *m_simTimer = nullptr;
    int m_simProgress = 0;
    bool m_transforming = false;
};

} // namespace processing::gui

#endif
