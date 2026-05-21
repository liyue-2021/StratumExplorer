#ifndef PROCESSING_GUI_DATA_MERGE_DIALOG_H
#define PROCESSING_GUI_DATA_MERGE_DIALOG_H

#include <QDialog>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
namespace Ui
{
class DataMergeDialog;
}
QT_END_NAMESPACE

class QTimer;
class QComboBox;

namespace processing::gui
{

/** @brief 「数据合并」节点配置弹窗（界面见 DataMergeDialog.ui） */
class DataMergeDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DataMergeDialog(QWidget *parent = nullptr);
    ~DataMergeDialog() override;

    void setParams(const QVariantMap &params);
    QVariantMap params() const;

signals:
    void paramsCommitted(const QVariantMap &params);
    void mergeSucceeded(const QVariantMap &params);
    void mergeFailed(const QString &message);

private slots:
    void onPickFolder();
    void onPickFile();
    void onPickOutput();
    void onModeChanged(int index);
    void onFilterSuffixChanged(int index);
    void onMerge();
    void onSimulateProgress();

private:
    void setupConnections();
    void setComboByText(QComboBox *combo, const QString &text) const;
    void updateInputMode();
    void updateSuffixEnabled();
    bool validateInputs(QString *errorMessage) const;
    void setMerging(bool merging);
    void setStatus(const QString &text, bool isError = false);

    Ui::DataMergeDialog *ui = nullptr;
    QTimer *m_simTimer = nullptr;
    int m_simProgress = 0;
    bool m_merging = false;
};

} // namespace processing::gui

#endif
