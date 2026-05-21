#include "DataCropDialog.h"
#include "ui_DataCropDialog.h"

#include <QComboBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QTimer>

using namespace processing::gui;

namespace
{
bool isOn(const QString &text)
{
    return text.compare(QStringLiteral("On"), Qt::CaseInsensitive) == 0;
}
} // namespace

DataCropDialog::DataCropDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DataCropDialog)
{
    ui->setupUi(this);

    if (auto *grid = ui->gridLayout)
    {
        grid->setColumnStretch(0, 0);
        grid->setColumnStretch(1, 1);
        grid->setColumnStretch(2, 0);
        grid->setColumnStretch(3, 1);
        grid->setColumnMinimumWidth(0, 96);
        grid->setColumnMinimumWidth(2, 96);
        grid->setHorizontalSpacing(10);
        grid->setVerticalSpacing(12);
    }

    ui->spinDepthMax->setValue(1000.0);
    ui->spinTimeMax->setValue(1000.0);

    m_simTimer = new QTimer(this);
    m_simTimer->setInterval(120);

    setupConnections();
    updateSuffixEnabled();
    updateChannelRangeEnabled();
    updateSampleRangeEnabled();
}

DataCropDialog::~DataCropDialog()
{
    delete ui;
}

void DataCropDialog::setupConnections()
{
    connect(ui->btnPickInput, &QPushButton::clicked, this, &DataCropDialog::onPickInput);
    connect(ui->btnPickOutput, &QPushButton::clicked, this, &DataCropDialog::onPickOutput);
    connect(ui->btnCrop, &QPushButton::clicked, this, &DataCropDialog::onCrop);
    connect(m_simTimer, &QTimer::timeout, this, &DataCropDialog::onSimulateProgress);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(ui->comboFilterSuffix, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DataCropDialog::onFilterSuffixChanged);
    connect(ui->comboChannelClip, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DataCropDialog::onChannelClipChanged);
    connect(ui->comboSampleClip, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DataCropDialog::onSampleClipChanged);
}

void DataCropDialog::setComboByText(QComboBox *combo, const QString &text) const
{
    if (!combo)
        return;
    const int idx = combo->findText(text);
    if (idx >= 0)
        combo->setCurrentIndex(idx);
}

void DataCropDialog::onFilterSuffixChanged(int)
{
    updateSuffixEnabled();
}

void DataCropDialog::onChannelClipChanged(int)
{
    updateChannelRangeEnabled();
}

void DataCropDialog::onSampleClipChanged(int)
{
    updateSampleRangeEnabled();
}

void DataCropDialog::updateSuffixEnabled()
{
    const bool enabled = isOn(ui->comboFilterSuffix->currentText());
    ui->comboSuffix->setEnabled(enabled);
    ui->labelSuffix->setEnabled(enabled);
}

void DataCropDialog::updateChannelRangeEnabled()
{
    const bool enabled = isOn(ui->comboChannelClip->currentText());
    ui->comboChannelMode->setEnabled(enabled);
    ui->labelChannelMode->setEnabled(enabled);
    ui->spinDepthMin->setEnabled(enabled);
    ui->spinDepthMax->setEnabled(enabled);
    ui->labelDepthMin->setEnabled(enabled);
    ui->labelDepthMax->setEnabled(enabled);
}

void DataCropDialog::updateSampleRangeEnabled()
{
    const bool enabled = isOn(ui->comboSampleClip->currentText());
    ui->comboSampleMode->setEnabled(enabled);
    ui->labelSampleMode->setEnabled(enabled);
    ui->spinTimeMin->setEnabled(enabled);
    ui->spinTimeMax->setEnabled(enabled);
    ui->labelTimeMin->setEnabled(enabled);
    ui->labelTimeMax->setEnabled(enabled);
}

void DataCropDialog::onPickInput()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("选择文件"), ui->editInputPath->text());
    if (!path.isEmpty())
        ui->editInputPath->setText(path);
}

void DataCropDialog::onPickOutput()
{
    const QString path = QFileDialog::getExistingDirectory(
        this, tr("选择输出文件夹"), ui->editOutputPath->text());
    if (!path.isEmpty())
        ui->editOutputPath->setText(path);
}

void DataCropDialog::setParams(const QVariantMap &params)
{
    ui->editInputPath->setText(params.value(QStringLiteral("input_path")).toString());
    setComboByText(ui->comboFilterSuffix,
                   params.value(QStringLiteral("filter_suffix"), QStringLiteral("Off")).toString());
    setComboByText(ui->comboSuffix,
                   params.value(QStringLiteral("suffix"), QStringLiteral(".osa")).toString());
    setComboByText(ui->comboFormat,
                   params.value(QStringLiteral("format"), QStringLiteral("OSA by OptaSoft")).toString());
    setComboByText(ui->comboDasFormat,
                   params.value(QStringLiteral("das_data_format"), QStringLiteral("Raw")).toString());
    setComboByText(ui->comboChannelClip,
                   params.value(QStringLiteral("channel_clip"), QStringLiteral("On")).toString());
    setComboByText(ui->comboChannelMode,
                   params.value(QStringLiteral("channel_mode"), QStringLiteral("Length")).toString());
    ui->spinDepthMin->setValue(params.value(QStringLiteral("depth_min"), 0.0).toDouble());
    ui->spinDepthMax->setValue(params.value(QStringLiteral("depth_max"), 1000.0).toDouble());
    setComboByText(ui->comboSampleClip,
                   params.value(QStringLiteral("sample_clip"), QStringLiteral("On")).toString());
    setComboByText(ui->comboSampleMode,
                   params.value(QStringLiteral("sample_mode"), QStringLiteral("Time")).toString());
    ui->spinTimeMin->setValue(params.value(QStringLiteral("time_min"), 0.0).toDouble());
    ui->spinTimeMax->setValue(params.value(QStringLiteral("time_max"), 1000.0).toDouble());
    ui->editOutputPath->setText(params.value(QStringLiteral("output_path")).toString());

    updateSuffixEnabled();
    updateChannelRangeEnabled();
    updateSampleRangeEnabled();
}

QVariantMap DataCropDialog::params() const
{
    QVariantMap p;
    p.insert(QStringLiteral("input_path"), ui->editInputPath->text().trimmed());
    p.insert(QStringLiteral("filter_suffix"), ui->comboFilterSuffix->currentText());
    p.insert(QStringLiteral("suffix"), ui->comboSuffix->currentText());
    p.insert(QStringLiteral("format"), ui->comboFormat->currentText());
    p.insert(QStringLiteral("das_data_format"), ui->comboDasFormat->currentText());
    p.insert(QStringLiteral("channel_clip"), ui->comboChannelClip->currentText());
    p.insert(QStringLiteral("channel_mode"), ui->comboChannelMode->currentText());
    p.insert(QStringLiteral("depth_min"), ui->spinDepthMin->value());
    p.insert(QStringLiteral("depth_max"), ui->spinDepthMax->value());
    p.insert(QStringLiteral("sample_clip"), ui->comboSampleClip->currentText());
    p.insert(QStringLiteral("sample_mode"), ui->comboSampleMode->currentText());
    p.insert(QStringLiteral("time_min"), ui->spinTimeMin->value());
    p.insert(QStringLiteral("time_max"), ui->spinTimeMax->value());
    p.insert(QStringLiteral("output_path"), ui->editOutputPath->text().trimmed());
    return p;
}

bool DataCropDialog::validateInputs(QString *errorMessage) const
{
    if (ui->editInputPath->text().trimmed().isEmpty())
    {
        if (errorMessage)
            *errorMessage = tr("请选择输入文件");
        return false;
    }
    if (ui->editOutputPath->text().trimmed().isEmpty())
    {
        if (errorMessage)
            *errorMessage = tr("请选择输出文件夹");
        return false;
    }
    if (isOn(ui->comboChannelClip->currentText())
        && ui->spinDepthMin->value() > ui->spinDepthMax->value())
    {
        if (errorMessage)
            *errorMessage = tr("深度最小值不能大于最大值");
        return false;
    }
    if (isOn(ui->comboSampleClip->currentText())
        && ui->spinTimeMin->value() > ui->spinTimeMax->value())
    {
        if (errorMessage)
            *errorMessage = tr("时间最小值不能大于最大值");
        return false;
    }
    return true;
}

void DataCropDialog::setCropping(bool cropping)
{
    m_cropping = cropping;
    ui->btnCrop->setEnabled(!cropping);
    ui->btnPickInput->setEnabled(!cropping);
    ui->btnPickOutput->setEnabled(!cropping);
    ui->editInputPath->setEnabled(!cropping);
    ui->editOutputPath->setEnabled(!cropping);
    ui->comboFilterSuffix->setEnabled(!cropping);
    ui->comboSuffix->setEnabled(!cropping && isOn(ui->comboFilterSuffix->currentText()));
    ui->comboFormat->setEnabled(!cropping);
    ui->comboDasFormat->setEnabled(!cropping);
    ui->comboChannelClip->setEnabled(!cropping);
    ui->comboChannelMode->setEnabled(!cropping && isOn(ui->comboChannelClip->currentText()));
    ui->spinDepthMin->setEnabled(!cropping && isOn(ui->comboChannelClip->currentText()));
    ui->spinDepthMax->setEnabled(!cropping && isOn(ui->comboChannelClip->currentText()));
    ui->comboSampleClip->setEnabled(!cropping);
    ui->comboSampleMode->setEnabled(!cropping && isOn(ui->comboSampleClip->currentText()));
    ui->spinTimeMin->setEnabled(!cropping && isOn(ui->comboSampleClip->currentText()));
    ui->spinTimeMax->setEnabled(!cropping && isOn(ui->comboSampleClip->currentText()));
    ui->btnCrop->setText(cropping ? tr("正在裁剪") : tr("裁剪"));
}

void DataCropDialog::setStatus(const QString &text, bool isError)
{
    ui->labelStatus->setText(text);
    if (isError)
        ui->labelStatus->setStyleSheet(
            QStringLiteral("color:#C62828; padding: 4px 0; font-weight: bold;"));
    else
        ui->labelStatus->setStyleSheet(QStringLiteral("color:#546E7A; padding: 4px 0;"));
}

void DataCropDialog::onCrop()
{
    if (m_cropping)
        return;

    QString err;
    if (!validateInputs(&err))
    {
        setStatus(err, true);
        return;
    }

    emit paramsCommitted(params());

    m_simProgress = 0;
    setCropping(true);
    setStatus(tr("正在裁剪 0%"));
    m_simTimer->start();
}

void DataCropDialog::onSimulateProgress()
{
    m_simProgress += 5;
    if (m_simProgress > 100)
        m_simProgress = 100;

    setStatus(tr("正在裁剪 %1%").arg(m_simProgress));

    if (m_simProgress < 100)
        return;

    m_simTimer->stop();
    setCropping(false);

    // TODO: 对接数据裁剪后端（func_data_crop）
    setStatus(tr("裁剪完成"), false);
    emit cropSucceeded(params());
}
