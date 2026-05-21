#ifndef PROCESSING_GUI_TIME_CORRECT_DIALOG_H
#define PROCESSING_GUI_TIME_CORRECT_DIALOG_H

#include <QDialog>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
namespace Ui
{
class TimeCorrectDialog;
}
QT_END_NAMESPACE

class QTimer;
class QComboBox;

namespace processing::gui
{

/** @brief 「时间矫正」节点配置弹窗（界面见 TimeCorrectDialog.ui） */
class TimeCorrectDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TimeCorrectDialog(QWidget *parent = nullptr);
    ~TimeCorrectDialog() override;

    void setParams(const QVariantMap &params);
    QVariantMap params() const;

signals:
    void paramsCommitted(const QVariantMap &params);
    void correctionSucceeded(const QVariantMap &params);
    void correctionFailed(const QString &message);

private slots:
    void onPickInput();
    void onPickOutput();
    void onCorrect();
    void onSimulateProgress();

private:
    void setupConnections();
    void setComboByText(QComboBox *combo, const QString &text) const;
    bool validateInputs(QString *errorMessage) const;
    void setCorrecting(bool correcting);
    void setStatus(const QString &text, bool isError = false);

    Ui::TimeCorrectDialog *ui = nullptr;
    QTimer *m_simTimer = nullptr;
    int m_simProgress = 0;
    bool m_correcting = false;
};

} // namespace processing::gui

#endif
