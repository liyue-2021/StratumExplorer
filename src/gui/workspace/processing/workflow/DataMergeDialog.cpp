#include "DataMergeDialog.h"
#include "ui_DataMergeDialog.h"

#include <QComboBox>
#include <QFileDialog>
#include <QTimer>

using namespace processing::gui;

namespace
{
bool isOn(const QString &text)
{
    return text.compare(QStringLiteral("On"), Qt::CaseInsensitive) == 0;
}

bool isDirectoryMode(const QString &text)
{
    return text.compare(QStringLiteral("Directory"), Qt::CaseInsensitive) == 0;
}
} // namespace

DataMergeDialog::DataMergeDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DataMergeDialog)
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

    ui->spinChannelCount->setValue(100);
    ui->spinSampleCount->setValue(60000);
    ui->spinChannelEnd->setValue(100);

    m_simTimer = new QTimer(this);
    m_simTimer->setInterval(120);

    setupConnections();
    updateInputMode();
    updateSuffixEnabled();
}

DataMergeDialog::~DataMergeDialog()
{
    delete ui;
}

void DataMergeDialog::setupConnections()
{
    connect(ui->btnPickFolder, &QPushButton::clicked, this, &DataMergeDialog::onPickFolder);
    connect(ui->btnPickFile, &QPushButton::clicked, this, &DataMergeDialog::onPickFile);
    connect(ui->btnPickOutput, &QPushButton::clicked, this, &DataMergeDialog::onPickOutput);
    connect(ui->btnMerge, &QPushButton::clicked, this, &DataMergeDialog::onMerge);
    connect(m_simTimer, &QTimer::timeout, this, &DataMergeDialog::onSimulateProgress);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(ui->comboMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DataMergeDialog::onModeChanged);
    connect(ui->comboFilterSuffix, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DataMergeDialog::onFilterSuffixChanged);
}

void DataMergeDialog::setComboByText(QComboBox *combo, const QString &text) const
{
    if (!combo)
        return;
    const int idx = combo->findText(text);
    if (idx >= 0)
        combo->setCurrentIndex(idx);
}

void DataMergeDialog::onModeChanged(int)
{
    updateInputMode();
}

void DataMergeDialog::onFilterSuffixChanged(int)
{
    updateSuffixEnabled();
}

void DataMergeDialog::updateInputMode()
{
    const bool dirMode = isDirectoryMode(ui->comboMode->currentText());
    ui->btnPickFolder->setEnabled(dirMode);
    ui->editFolderPath->setEnabled(dirMode);
    ui->btnPickFile->setEnabled(!dirMode);
    ui->editFilePath->setEnabled(!dirMode);
}

void DataMergeDialog::updateSuffixEnabled()
{
    const bool enabled = isOn(ui->comboFilterSuffix->currentText());
    ui->comboSuffix->setEnabled(enabled);
    ui->labelSuffix->setEnabled(enabled);
}

void DataMergeDialog::onPickFolder()
{
    const QString path = QFileDialog::getExistingDirectory(
        this, tr("选择文件夹"), ui->editFolderPath->text());
    if (!path.isEmpty())
        ui->editFolderPath->setText(path);
}

void DataMergeDialog::onPickFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("选择文件"), ui->editFilePath->text());
    if (!path.isEmpty())
        ui->editFilePath->setText(path);
}

void DataMergeDialog::onPickOutput()
{
    const QString path = QFileDialog::getExistingDirectory(
        this, tr("选择输出文件夹"), ui->editOutputPath->text());
    if (!path.isEmpty())
        ui->editOutputPath->setText(path);
}

void DataMergeDialog::setParams(const QVariantMap &params)
{
    setComboByText(ui->comboMode,
                   params.value(QStringLiteral("mode"), QStringLiteral("Directory")).toString());
    ui->editFolderPath->setText(params.value(QStringLiteral("input_folder")).toString());
    ui->editFilePath->setText(params.value(QStringLiteral("input_file")).toString());
    setComboByText(ui->comboFilterSuffix,
                   params.value(QStringLiteral("filter_suffix"), QStringLiteral("Off")).toString());
    setComboByText(ui->comboSuffix,
                   params.value(QStringLiteral("suffix"), QStringLiteral(".osa")).toString());
    setComboByText(ui->comboPriority,
                   params.value(QStringLiteral("processing_priority"), QStringLiteral("Precision"))
                       .toString());
    setComboByText(ui->comboFirstDim,
                   params.value(QStringLiteral("first_dimension"), QStringLiteral("Time")).toString());
    setComboByText(ui->comboFillMissing,
                   params.value(QStringLiteral("fill_missing"), QStringLiteral("On")).toString());
    ui->spinChannelCount->setValue(params.value(QStringLiteral("channel_count"), 100).toInt());
    ui->spinSampleCount->setValue(params.value(QStringLiteral("sample_count"), 60000).toInt());
    ui->spinChannelStart->setValue(params.value(QStringLiteral("channel_start"), 0).toInt());
    ui->spinChannelEnd->setValue(params.value(QStringLiteral("channel_end"), 100).toInt());
    ui->editOutputName->setText(
        params.value(QStringLiteral("output_filename"), QStringLiteral("merged_")).toString());
    ui->editOutputPath->setText(params.value(QStringLiteral("output_path")).toString());

    updateInputMode();
    updateSuffixEnabled();
}

QVariantMap DataMergeDialog::params() const
{
    QVariantMap p;
    p.insert(QStringLiteral("mode"), ui->comboMode->currentText());
    p.insert(QStringLiteral("input_folder"), ui->editFolderPath->text().trimmed());
    p.insert(QStringLiteral("input_file"), ui->editFilePath->text().trimmed());
    p.insert(QStringLiteral("filter_suffix"), ui->comboFilterSuffix->currentText());
    p.insert(QStringLiteral("suffix"), ui->comboSuffix->currentText());
    p.insert(QStringLiteral("processing_priority"), ui->comboPriority->currentText());
    p.insert(QStringLiteral("first_dimension"), ui->comboFirstDim->currentText());
    p.insert(QStringLiteral("fill_missing"), ui->comboFillMissing->currentText());
    p.insert(QStringLiteral("channel_count"), ui->spinChannelCount->value());
    p.insert(QStringLiteral("sample_count"), ui->spinSampleCount->value());
    p.insert(QStringLiteral("channel_start"), ui->spinChannelStart->value());
    p.insert(QStringLiteral("channel_end"), ui->spinChannelEnd->value());
    p.insert(QStringLiteral("output_filename"), ui->editOutputName->text().trimmed());
    p.insert(QStringLiteral("output_path"), ui->editOutputPath->text().trimmed());
    return p;
}

bool DataMergeDialog::validateInputs(QString *errorMessage) const
{
    if (isDirectoryMode(ui->comboMode->currentText()))
    {
        if (ui->editFolderPath->text().trimmed().isEmpty())
        {
            if (errorMessage)
                *errorMessage = tr("请选择输入文件夹");
            return false;
        }
    }
    else if (ui->editFilePath->text().trimmed().isEmpty())
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
    if (ui->editOutputName->text().trimmed().isEmpty())
    {
        if (errorMessage)
            *errorMessage = tr("请填写输出文件名称");
        return false;
    }
    if (ui->spinChannelStart->value() > ui->spinChannelEnd->value())
    {
        if (errorMessage)
            *errorMessage = tr("通道起始值不能大于终止值");
        return false;
    }
    return true;
}

void DataMergeDialog::setMerging(bool merging)
{
    m_merging = merging;
    ui->btnMerge->setEnabled(!merging);
    ui->btnPickFolder->setEnabled(!merging && isDirectoryMode(ui->comboMode->currentText()));
    ui->btnPickFile->setEnabled(!merging && !isDirectoryMode(ui->comboMode->currentText()));
    ui->btnPickOutput->setEnabled(!merging);
    ui->comboMode->setEnabled(!merging);
    ui->editFolderPath->setEnabled(!merging && isDirectoryMode(ui->comboMode->currentText()));
    ui->editFilePath->setEnabled(!merging && !isDirectoryMode(ui->comboMode->currentText()));
    ui->comboFilterSuffix->setEnabled(!merging);
    ui->comboSuffix->setEnabled(!merging && isOn(ui->comboFilterSuffix->currentText()));
    ui->comboPriority->setEnabled(!merging);
    ui->comboFirstDim->setEnabled(!merging);
    ui->comboFillMissing->setEnabled(!merging);
    ui->spinChannelCount->setEnabled(!merging);
    ui->spinSampleCount->setEnabled(!merging);
    ui->spinChannelStart->setEnabled(!merging);
    ui->spinChannelEnd->setEnabled(!merging);
    ui->editOutputName->setEnabled(!merging);
    ui->editOutputPath->setEnabled(!merging);
    ui->btnMerge->setText(merging ? tr("正在合并") : tr("合并"));
}

void DataMergeDialog::setStatus(const QString &text, bool isError)
{
    ui->labelStatus->setText(text);
    if (isError)
        ui->labelStatus->setStyleSheet(
            QStringLiteral("color:#C62828; padding: 4px 0; font-weight: bold;"));
    else
        ui->labelStatus->setStyleSheet(QStringLiteral("color:#546E7A; padding: 4px 0;"));
}

void DataMergeDialog::onMerge()
{
    if (m_merging)
        return;

    QString err;
    if (!validateInputs(&err))
    {
        setStatus(err, true);
        return;
    }

    emit paramsCommitted(params());

    m_simProgress = 0;
    setMerging(true);
    setStatus(tr("正在合并 0%"));
    m_simTimer->start();
}

void DataMergeDialog::onSimulateProgress()
{
    m_simProgress += 5;
    if (m_simProgress > 100)
        m_simProgress = 100;

    setStatus(tr("正在合并 %1%").arg(m_simProgress));

    if (m_simProgress < 100)
        return;

    m_simTimer->stop();
    setMerging(false);

    // TODO: 对接数据合并后端（func_data_merge）
    setStatus(tr("合并完成"), false);
    emit mergeSucceeded(params());
}
