#include "LfdasExtractDialog.h"
#include "ui_LfdasExtractDialog.h"

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

LfdasExtractDialog::LfdasExtractDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LfdasExtractDialog)
{
    ui->setupUi(this);

    if (auto *grid = ui->gridLayout)
    {
        for (int col = 0; col < 6; ++col)
        {
            const bool isLabelCol = (col % 2) == 0;
            grid->setColumnStretch(col, isLabelCol ? 0 : 1);
            if (isLabelCol)
                grid->setColumnMinimumWidth(col, 80);
        }
    }

    ui->spinFHigh->setValue(1.0);
    ui->spinCorner->setValue(4);
    ui->spinDownFactor->setValue(10);
    ui->spinFirOrder->setValue(200);
    ui->spinTimeWinLen->setValue(1000000);
    ui->spinChannelWinLen->setValue(5);

    m_simTimer = new QTimer(this);
    m_simTimer->setInterval(120);

    setupConnections();
    updateSuffixEnabled();
    updateTimeFilterEnabled();
    updateDepthFilterEnabled();
}

LfdasExtractDialog::~LfdasExtractDialog()
{
    delete ui;
}

void LfdasExtractDialog::setupConnections()
{
    connect(ui->btnPickInput, &QPushButton::clicked, this, &LfdasExtractDialog::onPickInput);
    connect(ui->btnPickOutput, &QPushButton::clicked, this, &LfdasExtractDialog::onPickOutput);
    connect(ui->btnExtract, &QPushButton::clicked, this, &LfdasExtractDialog::onExtract);
    connect(m_simTimer, &QTimer::timeout, this, &LfdasExtractDialog::onSimulateProgress);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(ui->comboFilterSuffix, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &LfdasExtractDialog::onFilterSuffixChanged);
    connect(ui->comboTimeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &LfdasExtractDialog::onTimeFilterChanged);
    connect(ui->comboDepthFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &LfdasExtractDialog::onDepthFilterChanged);
}

void LfdasExtractDialog::setComboByText(QComboBox *combo, const QString &text) const
{
    if (!combo)
        return;
    const int idx = combo->findText(text);
    if (idx >= 0)
        combo->setCurrentIndex(idx);
}

void LfdasExtractDialog::onFilterSuffixChanged(int)
{
    updateSuffixEnabled();
}

void LfdasExtractDialog::onTimeFilterChanged(int)
{
    updateTimeFilterEnabled();
}

void LfdasExtractDialog::onDepthFilterChanged(int)
{
    updateDepthFilterEnabled();
}

void LfdasExtractDialog::updateSuffixEnabled()
{
    const bool enabled = isOn(ui->comboFilterSuffix->currentText());
    ui->comboSuffix->setEnabled(enabled);
    ui->labelSuffix->setEnabled(enabled);
}

void LfdasExtractDialog::updateTimeFilterEnabled()
{
    const bool enabled = isOn(ui->comboTimeFilter->currentText());
    ui->comboTimeFilterType->setEnabled(enabled);
    ui->labelTimeFilterType->setEnabled(enabled);
    ui->spinDataAxis->setEnabled(enabled);
    ui->labelDataAxis->setEnabled(enabled);
    ui->spinDownFactor->setEnabled(enabled);
    ui->labelDownFactor->setEnabled(enabled);
    ui->comboFirType->setEnabled(enabled);
    ui->labelFirType->setEnabled(enabled);
    ui->spinFirOrder->setEnabled(enabled);
    ui->labelFirOrder->setEnabled(enabled);
    ui->comboZeroPhaseTime->setEnabled(enabled);
    ui->labelZeroPhaseTime->setEnabled(enabled);
    ui->spinTimeWinLen->setEnabled(enabled);
    ui->labelTimeWinLen->setEnabled(enabled);
    ui->spinTimeOverlap->setEnabled(enabled);
    ui->labelTimeOverlap->setEnabled(enabled);
}

void LfdasExtractDialog::updateDepthFilterEnabled()
{
    const bool enabled = isOn(ui->comboDepthFilter->currentText());
    ui->comboDepthFilterType->setEnabled(enabled);
    ui->labelDepthFilterType->setEnabled(enabled);
    ui->spinChannelWinLen->setEnabled(enabled);
    ui->labelChannelWinLen->setEnabled(enabled);
}

void LfdasExtractDialog::onPickInput()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("选择文件"), ui->editInputPath->text());
    if (!path.isEmpty())
        ui->editInputPath->setText(path);
}

void LfdasExtractDialog::onPickOutput()
{
    const QString path = QFileDialog::getExistingDirectory(
        this, tr("选择输出文件夹"), ui->editOutputPath->text());
    if (!path.isEmpty())
        ui->editOutputPath->setText(path);
}

void LfdasExtractDialog::setParams(const QVariantMap &params)
{
    ui->editInputPath->setText(params.value(QStringLiteral("input_path")).toString());
    setComboByText(ui->comboFilterSuffix,
                   params.value(QStringLiteral("filter_suffix"), QStringLiteral("Off")).toString());
    setComboByText(ui->comboSuffix,
                   params.value(QStringLiteral("suffix"), QStringLiteral(".osa")).toString());
    setComboByText(ui->comboFormat,
                   params.value(QStringLiteral("format"), QStringLiteral("OSA by OptaSoft")).toString());
    setComboByText(ui->comboDataType,
                   params.value(QStringLiteral("data_type"), QStringLiteral("Raw")).toString());
    setComboByText(ui->comboFilterMode,
                   params.value(QStringLiteral("filter_mode"), QStringLiteral("Lowpass")).toString());
    ui->spinFLow->setValue(params.value(QStringLiteral("f_low"), 0.0).toDouble());
    ui->spinFHigh->setValue(params.value(QStringLiteral("f_high"), 1.0).toDouble());
    ui->spinCorner->setValue(params.value(QStringLiteral("corner"), 4).toInt());
    setComboByText(ui->comboZeroPhaseMode,
                   params.value(QStringLiteral("zero_phase_mode"), QStringLiteral("True")).toString());
    setComboByText(ui->comboTimeFilter,
                   params.value(QStringLiteral("time_filter"), QStringLiteral("On")).toString());
    setComboByText(ui->comboTimeFilterType,
                   params.value(QStringLiteral("time_filter_type"), QStringLiteral("Averaging"))
                       .toString());
    ui->spinDataAxis->setValue(params.value(QStringLiteral("data_axis"), 0).toInt());
    ui->spinDownFactor->setValue(params.value(QStringLiteral("down_factor"), 10).toInt());
    setComboByText(ui->comboFirType,
                   params.value(QStringLiteral("fir_type"), QStringLiteral("fir")).toString());
    ui->spinFirOrder->setValue(params.value(QStringLiteral("fir_order"), 200).toInt());
    setComboByText(ui->comboZeroPhaseTime,
                   params.value(QStringLiteral("zero_phase_time"), QStringLiteral("True")).toString());
    ui->spinTimeWinLen->setValue(params.value(QStringLiteral("time_win_len_us"), 1000000).toInt());
    ui->spinTimeOverlap->setValue(params.value(QStringLiteral("time_overlap_us"), 0).toInt());
    setComboByText(ui->comboDepthFilter,
                   params.value(QStringLiteral("depth_filter"), QStringLiteral("On")).toString());
    setComboByText(ui->comboDepthFilterType,
                   params.value(QStringLiteral("depth_filter_type"), QStringLiteral("Median")).toString());
    ui->spinChannelWinLen->setValue(params.value(QStringLiteral("channel_win_len"), 5).toInt());
    setComboByText(ui->comboIoParallel,
                   params.value(QStringLiteral("io_parallel"), QStringLiteral("Off")).toString());
    setComboByText(ui->comboNaming,
                   params.value(QStringLiteral("naming"), QStringLiteral("DateTime")).toString());
    ui->editOutputPath->setText(params.value(QStringLiteral("output_path")).toString());

    updateSuffixEnabled();
    updateTimeFilterEnabled();
    updateDepthFilterEnabled();
}

QVariantMap LfdasExtractDialog::params() const
{
    QVariantMap p;
    p.insert(QStringLiteral("input_path"), ui->editInputPath->text().trimmed());
    p.insert(QStringLiteral("filter_suffix"), ui->comboFilterSuffix->currentText());
    p.insert(QStringLiteral("suffix"), ui->comboSuffix->currentText());
    p.insert(QStringLiteral("format"), ui->comboFormat->currentText());
    p.insert(QStringLiteral("data_type"), ui->comboDataType->currentText());
    p.insert(QStringLiteral("filter_mode"), ui->comboFilterMode->currentText());
    p.insert(QStringLiteral("f_low"), ui->spinFLow->value());
    p.insert(QStringLiteral("f_high"), ui->spinFHigh->value());
    p.insert(QStringLiteral("corner"), ui->spinCorner->value());
    p.insert(QStringLiteral("zero_phase_mode"), ui->comboZeroPhaseMode->currentText());
    p.insert(QStringLiteral("time_filter"), ui->comboTimeFilter->currentText());
    p.insert(QStringLiteral("time_filter_type"), ui->comboTimeFilterType->currentText());
    p.insert(QStringLiteral("data_axis"), ui->spinDataAxis->value());
    p.insert(QStringLiteral("down_factor"), ui->spinDownFactor->value());
    p.insert(QStringLiteral("fir_type"), ui->comboFirType->currentText());
    p.insert(QStringLiteral("fir_order"), ui->spinFirOrder->value());
    p.insert(QStringLiteral("zero_phase_time"), ui->comboZeroPhaseTime->currentText());
    p.insert(QStringLiteral("time_win_len_us"), ui->spinTimeWinLen->value());
    p.insert(QStringLiteral("time_overlap_us"), ui->spinTimeOverlap->value());
    p.insert(QStringLiteral("depth_filter"), ui->comboDepthFilter->currentText());
    p.insert(QStringLiteral("depth_filter_type"), ui->comboDepthFilterType->currentText());
    p.insert(QStringLiteral("channel_win_len"), ui->spinChannelWinLen->value());
    p.insert(QStringLiteral("io_parallel"), ui->comboIoParallel->currentText());
    p.insert(QStringLiteral("naming"), ui->comboNaming->currentText());
    p.insert(QStringLiteral("output_path"), ui->editOutputPath->text().trimmed());
    return p;
}

bool LfdasExtractDialog::validateInputs(QString *errorMessage) const
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
    if (ui->spinFLow->value() > ui->spinFHigh->value())
    {
        if (errorMessage)
            *errorMessage = tr("低频截止不能大于高频截止");
        return false;
    }
    return true;
}

void LfdasExtractDialog::setExtracting(bool extracting)
{
    m_extracting = extracting;
    ui->btnExtract->setEnabled(!extracting);
    ui->btnPickInput->setEnabled(!extracting);
    ui->btnPickOutput->setEnabled(!extracting);
    if (auto *grid = ui->gridLayout)
    {
        for (int i = 0; i < grid->count(); ++i)
        {
            if (auto *item = grid->itemAt(i))
            {
                if (auto *w = item->widget())
                    w->setEnabled(!extracting);
            }
        }
    }
    ui->btnExtract->setText(extracting ? tr("正在提取") : tr("提取"));
}

void LfdasExtractDialog::setStatus(const QString &text, bool isError)
{
    ui->labelStatus->setText(text);
    if (isError)
        ui->labelStatus->setStyleSheet(
            QStringLiteral("color:#C62828; padding: 4px 0; font-weight: bold;"));
    else
        ui->labelStatus->setStyleSheet(QStringLiteral("color:#546E7A; padding: 4px 0;"));
}

void LfdasExtractDialog::onExtract()
{
    if (m_extracting)
        return;

    QString err;
    if (!validateInputs(&err))
    {
        setStatus(err, true);
        return;
    }

    emit paramsCommitted(params());

    m_simProgress = 0;
    setExtracting(true);
    setStatus(tr("正在提取 0%"));
    m_simTimer->start();
}

void LfdasExtractDialog::onSimulateProgress()
{
    m_simProgress += 5;
    if (m_simProgress > 100)
        m_simProgress = 100;

    setStatus(tr("正在提取 %1%").arg(m_simProgress));

    if (m_simProgress < 100)
        return;

    m_simTimer->stop();
    setExtracting(false);

    // TODO: 对接 LF-DAS 提取后端（func_lfdas_extract）
    setStatus(tr("提取完成"), false);
    emit extractSucceeded(params());
}
