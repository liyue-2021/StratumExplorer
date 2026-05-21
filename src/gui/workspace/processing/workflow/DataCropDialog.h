#ifndef PROCESSING_GUI_DATA_CROP_DIALOG_H
#define PROCESSING_GUI_DATA_CROP_DIALOG_H

#include <QDialog>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
namespace Ui
{
class DataCropDialog;
}
QT_END_NAMESPACE

class QTimer;
class QComboBox;

namespace processing::gui
{

/** @brief 「数据裁剪」节点配置弹窗（界面见 DataCropDialog.ui） */
class DataCropDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DataCropDialog(QWidget *parent = nullptr);
    ~DataCropDialog() override;

    void setParams(const QVariantMap &params);
    QVariantMap params() const;

signals:
    void paramsCommitted(const QVariantMap &params);
    void cropSucceeded(const QVariantMap &params);
    void cropFailed(const QString &message);

private slots:
    void onPickInput();
    void onPickOutput();
    void onFilterSuffixChanged(int index);
    void onChannelClipChanged(int index);
    void onSampleClipChanged(int index);
    void onCrop();
    void onSimulateProgress();

private:
    void setupConnections();
    void setComboByText(QComboBox *combo, const QString &text) const;
    void updateSuffixEnabled();
    void updateChannelRangeEnabled();
    void updateSampleRangeEnabled();
    bool validateInputs(QString *errorMessage) const;
    void setCropping(bool cropping);
    void setStatus(const QString &text, bool isError = false);

    Ui::DataCropDialog *ui = nullptr;
    QTimer *m_simTimer = nullptr;
    int m_simProgress = 0;
    bool m_cropping = false;
};

} // namespace processing::gui

#endif
