#ifndef PROCESSING_GUI_LFDAS_EXTRACT_DIALOG_H
#define PROCESSING_GUI_LFDAS_EXTRACT_DIALOG_H

#include <QDialog>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
namespace Ui
{
class LfdasExtractDialog;
}
QT_END_NAMESPACE

class QTimer;
class QComboBox;

namespace processing::gui
{

/** @brief 「LF-DAS提取」节点配置弹窗（界面见 LfdasExtractDialog.ui） */
class LfdasExtractDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LfdasExtractDialog(QWidget *parent = nullptr);
    ~LfdasExtractDialog() override;

    void setParams(const QVariantMap &params);
    QVariantMap params() const;

signals:
    void paramsCommitted(const QVariantMap &params);
    void extractSucceeded(const QVariantMap &params);
    void extractFailed(const QString &message);

private slots:
    void onPickInput();
    void onPickOutput();
    void onFilterSuffixChanged(int index);
    void onTimeFilterChanged(int index);
    void onDepthFilterChanged(int index);
    void onExtract();
    void onSimulateProgress();

private:
    void setupConnections();
    void setComboByText(QComboBox *combo, const QString &text) const;
    void updateSuffixEnabled();
    void updateTimeFilterEnabled();
    void updateDepthFilterEnabled();
    bool validateInputs(QString *errorMessage) const;
    void setExtracting(bool extracting);
    void setStatus(const QString &text, bool isError = false);

    Ui::LfdasExtractDialog *ui = nullptr;
    QTimer *m_simTimer = nullptr;
    int m_simProgress = 0;
    bool m_extracting = false;
};

} // namespace processing::gui

#endif
