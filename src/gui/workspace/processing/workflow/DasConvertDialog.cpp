#include "DasConvertDialog.h"
#include "ui_DasConvertDialog.h"

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
} // namespace

DasConvertDialog::DasConvertDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DasConvertDialog)
{
    ui->setupUi(this);

    ui->spinNormMax->setValue(1.0);
    ui->spinWinLen->setValue(1000000);

    m_simTimer = new QTimer(this);
    m_simTimer->setInterval(120);

    setupConnections();
    updateNormalizationEnabled();
}

DasConvertDialog::~DasConvertDialog()
{
    delete ui;
}

void DasConvertDialog::setupConnections()
{
    connect(ui->btnPickInput, &QPushButton::clicked, this, &DasConvertDialog::onPickInput);
    connect(ui->btnPickOutput, &QPushButton::clicked, this, &DasConvertDialog::onPickOutput);
    connect(ui->btnTransform, &QPushButton::clicked, this, &DasConvertDialog::onTransform);
    connect(m_simTimer, &QTimer::timeout, this, &DasConvertDialog::onSimulateProgress);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(ui->comboNormalization, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DasConvertDialog::onNormalizationChanged);
}

void DasConvertDialog::setComboByText(QComboBox *combo, const QString &text) const
{
    if (!combo)
        return;
    const int idx = combo->findText(text);
    if (idx >= 0)
        combo->setCurrentIndex(idx);
}

void DasConvertDialog::onNormalizationChanged(int)
{
    updateNormalizationEnabled();
}

void DasConvertDialog::updateNormalizationEnabled()
{
    const bool enabled = isOn(ui->comboNormalization->currentText());
    ui->comboNormType->setEnabled(enabled);
    ui->labelNormType->setEnabled(enabled);
    ui->spinNormMin->setEnabled(enabled);
    ui->spinNormMax->setEnabled(enabled);
    ui->labelNormMin->setEnabled(enabled);
    ui->labelNormMax->setEnabled(enabled);
}

void DasConvertDialog::onPickInput()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("选择文件"), ui->editInputPath->text());
    if (!path.isEmpty())
        ui->editInputPath->setText(path);
}

void DasConvertDialog::onPickOutput()
{
    const QString path = QFileDialog::getExistingDirectory(
        this, tr("选择输出文件夹"), ui->editOutputPath->text());
    if (!path.isEmpty())
        ui->editOutputPath->setText(path);
}

void DasConvertDialog::setParams(const QVariantMap &params)
{
    ui->editInputPath->setText(params.value(QStringLiteral("input_path")).toString());
    setComboByText(ui->comboFormat,
                   params.value(QStringLiteral("format"), QStringLiteral("OSA by OptaSoft")).toString());
    setComboByText(ui->comboDasDataType,
                   params.value(QStringLiteral("das_data_type"), QStringLiteral("Raw")).toString());
    setComboByText(ui->comboTransformMode,
                   params.value(QStringLiteral("transform_mode"), QStringLiteral("RMS")).toString());
    setComboByText(ui->comboValueUnit,
                   params.value(QStringLiteral("value_unit"), QStringLiteral("Magnitude")).toString());
    setComboByText(ui->comboNormalization,
                   params.value(QStringLiteral("normalization"), QStringLiteral("On")).toString());
    setComboByText(ui->comboNormType,
                   params.value(QStringLiteral("normalization_type"), QStringLiteral("Trace")).toString());
    ui->spinNormMin->setValue(params.value(QStringLiteral("norm_min"), 0.0).toDouble());
    ui->spinNormMax->setValue(params.value(QStringLiteral("norm_max"), 1.0).toDouble());
    setComboByText(ui->comboProcDim,
                   params.value(QStringLiteral("processing_dimension"), QStringLiteral("Time Only"))
                       .toString());
    ui->spinWinLen->setValue(params.value(QStringLiteral("win_len_us"), 1000000).toInt());
    ui->spinOverlapLen->setValue(params.value(QStringLiteral("overlap_len_us"), 0).toInt());
    ui->editOutputPath->setText(params.value(QStringLiteral("output_path")).toString());

    updateNormalizationEnabled();
}

QVariantMap DasConvertDialog::params() const
{
    QVariantMap p;
    p.insert(QStringLiteral("input_path"), ui->editInputPath->text().trimmed());
    p.insert(QStringLiteral("format"), ui->comboFormat->currentText());
    p.insert(QStringLiteral("das_data_type"), ui->comboDasDataType->currentText());
    p.insert(QStringLiteral("transform_mode"), ui->comboTransformMode->currentText());
    p.insert(QStringLiteral("value_unit"), ui->comboValueUnit->currentText());
    p.insert(QStringLiteral("normalization"), ui->comboNormalization->currentText());
    p.insert(QStringLiteral("normalization_type"), ui->comboNormType->currentText());
    p.insert(QStringLiteral("norm_min"), ui->spinNormMin->value());
    p.insert(QStringLiteral("norm_max"), ui->spinNormMax->value());
    p.insert(QStringLiteral("processing_dimension"), ui->comboProcDim->currentText());
    p.insert(QStringLiteral("win_len_us"), ui->spinWinLen->value());
    p.insert(QStringLiteral("overlap_len_us"), ui->spinOverlapLen->value());
    p.insert(QStringLiteral("output_path"), ui->editOutputPath->text().trimmed());
    return p;
}

bool DasConvertDialog::validateInputs(QString *errorMessage) const
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
    if (isOn(ui->comboNormalization->currentText())
        && ui->spinNormMin->value() > ui->spinNormMax->value())
    {
        if (errorMessage)
            *errorMessage = tr("归一化最小值不能大于最大值");
        return false;
    }
    return true;
}

void DasConvertDialog::setTransforming(bool transforming)
{
    m_transforming = transforming;
    ui->btnTransform->setEnabled(!transforming);
    ui->btnPickInput->setEnabled(!transforming);
    ui->btnPickOutput->setEnabled(!transforming);
    ui->editInputPath->setEnabled(!transforming);
    ui->editOutputPath->setEnabled(!transforming);
    ui->comboFormat->setEnabled(!transforming);
    ui->comboDasDataType->setEnabled(!transforming);
    ui->comboTransformMode->setEnabled(!transforming);
    ui->comboValueUnit->setEnabled(!transforming);
    ui->comboNormalization->setEnabled(!transforming);
    ui->comboNormType->setEnabled(!transforming && isOn(ui->comboNormalization->currentText()));
    ui->spinNormMin->setEnabled(!transforming && isOn(ui->comboNormalization->currentText()));
    ui->spinNormMax->setEnabled(!transforming && isOn(ui->comboNormalization->currentText()));
    ui->comboProcDim->setEnabled(!transforming);
    ui->spinWinLen->setEnabled(!transforming);
    ui->spinOverlapLen->setEnabled(!transforming);
    ui->btnTransform->setText(transforming ? tr("正在变换") : tr("变换"));
}

void DasConvertDialog::setStatus(const QString &text, bool isError)
{
    ui->labelStatus->setText(text);
    if (isError)
        ui->labelStatus->setStyleSheet(
            QStringLiteral("color:#C62828; padding: 4px 0; font-weight: bold;"));
    else
        ui->labelStatus->setStyleSheet(QStringLiteral("color:#546E7A; padding: 4px 0;"));
}

void DasConvertDialog::onTransform()
{
    if (m_transforming)
        return;

    QString err;
    if (!validateInputs(&err))
    {
        setStatus(err, true);
        return;
    }

    emit paramsCommitted(params());

    m_simProgress = 0;
    setTransforming(true);
    setStatus(tr("正在变换 0%"));
    m_simTimer->start();
}

void DasConvertDialog::onSimulateProgress()
{
    m_simProgress += 5;
    if (m_simProgress > 100)
        m_simProgress = 100;

    setStatus(tr("正在变换 %1%").arg(m_simProgress));

    if (m_simProgress < 100)
        return;

    m_simTimer->stop();
    setTransforming(false);

    // TODO: 对接 DAS 数据转换后端（func_das_convert）
    setStatus(tr("变换完成"), false);
    emit transformSucceeded(params());
}
