#include "DepthCorrectDialog.h"
#include "ui_DepthCorrectDialog.h"

#include <QComboBox>
#include <QFileDialog>
#include <QTimer>

using namespace processing::gui;

DepthCorrectDialog::DepthCorrectDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DepthCorrectDialog)
{
    ui->setupUi(this);

    ui->spinDepthOffset->setValue(0.0);
    ui->spinDepthScale->setValue(0.98269);

    m_simTimer = new QTimer(this);
    m_simTimer->setInterval(120);

    setupConnections();
}

DepthCorrectDialog::~DepthCorrectDialog()
{
    delete ui;
}

void DepthCorrectDialog::setupConnections()
{
    connect(ui->btnPickInput, &QPushButton::clicked, this, &DepthCorrectDialog::onPickInput);
    connect(ui->btnPickOutput, &QPushButton::clicked, this, &DepthCorrectDialog::onPickOutput);
    connect(ui->btnCorrect, &QPushButton::clicked, this, &DepthCorrectDialog::onCorrect);
    connect(m_simTimer, &QTimer::timeout, this, &DepthCorrectDialog::onSimulateProgress);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void DepthCorrectDialog::setComboByText(QComboBox *combo, const QString &text) const
{
    if (!combo)
        return;
    const int idx = combo->findText(text);
    if (idx >= 0)
        combo->setCurrentIndex(idx);
}

void DepthCorrectDialog::onPickInput()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("选择文件"), ui->editInputPath->text());
    if (!path.isEmpty())
        ui->editInputPath->setText(path);
}

void DepthCorrectDialog::onPickOutput()
{
    const QString path = QFileDialog::getExistingDirectory(
        this, tr("选择输出文件夹"), ui->editOutputPath->text());
    if (!path.isEmpty())
        ui->editOutputPath->setText(path);
}

void DepthCorrectDialog::setParams(const QVariantMap &params)
{
    ui->editInputPath->setText(params.value(QStringLiteral("input_path")).toString());
    setComboByText(ui->comboFormat,
                   params.value(QStringLiteral("format"), QStringLiteral("OSA by OptaSoft")).toString());
    setComboByText(ui->comboDasDataType,
                   params.value(QStringLiteral("das_data_type"), QStringLiteral("Raw")).toString());
    ui->spinDepthOffset->setValue(params.value(QStringLiteral("depth_offset_m"), 0.0).toDouble());
    ui->spinDepthScale->setValue(params.value(QStringLiteral("depth_scale"), 0.98269).toDouble());
    setComboByText(ui->comboWriteBack,
                   params.value(QStringLiteral("write_back"), QStringLiteral("Off")).toString());
    setComboByText(ui->comboOutputFormat,
                   params.value(QStringLiteral("output_format"), QStringLiteral(".osa")).toString());
    setComboByText(ui->comboOutputNaming,
                   params.value(QStringLiteral("output_naming"), QStringLiteral("DateTime")).toString());
    ui->editOutputPath->setText(params.value(QStringLiteral("output_path")).toString());
}

QVariantMap DepthCorrectDialog::params() const
{
    QVariantMap p;
    p.insert(QStringLiteral("input_path"), ui->editInputPath->text().trimmed());
    p.insert(QStringLiteral("format"), ui->comboFormat->currentText());
    p.insert(QStringLiteral("das_data_type"), ui->comboDasDataType->currentText());
    p.insert(QStringLiteral("depth_offset_m"), ui->spinDepthOffset->value());
    p.insert(QStringLiteral("depth_scale"), ui->spinDepthScale->value());
    p.insert(QStringLiteral("write_back"), ui->comboWriteBack->currentText());
    p.insert(QStringLiteral("output_format"), ui->comboOutputFormat->currentText());
    p.insert(QStringLiteral("output_naming"), ui->comboOutputNaming->currentText());
    p.insert(QStringLiteral("output_path"), ui->editOutputPath->text().trimmed());
    return p;
}

bool DepthCorrectDialog::validateInputs(QString *errorMessage) const
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
    return true;
}

void DepthCorrectDialog::setCorrecting(bool correcting)
{
    m_correcting = correcting;
    ui->btnCorrect->setEnabled(!correcting);
    ui->btnPickInput->setEnabled(!correcting);
    ui->btnPickOutput->setEnabled(!correcting);
    ui->comboFormat->setEnabled(!correcting);
    ui->comboDasDataType->setEnabled(!correcting);
    ui->spinDepthOffset->setEnabled(!correcting);
    ui->spinDepthScale->setEnabled(!correcting);
    ui->comboWriteBack->setEnabled(!correcting);
    ui->comboOutputFormat->setEnabled(!correcting);
    ui->comboOutputNaming->setEnabled(!correcting);
    ui->editInputPath->setEnabled(!correcting);
    ui->editOutputPath->setEnabled(!correcting);
    ui->btnCorrect->setText(correcting ? tr("正在校正中") : tr("校正"));
}

void DepthCorrectDialog::setStatus(const QString &text, bool isError)
{
    ui->labelStatus->setText(text);
    if (isError)
        ui->labelStatus->setStyleSheet(
            QStringLiteral("color:#C62828; padding: 4px 0; font-weight: bold;"));
    else
        ui->labelStatus->setStyleSheet(QStringLiteral("color:#546E7A; padding: 4px 0;"));
}

void DepthCorrectDialog::onCorrect()
{
    if (m_correcting)
        return;

    QString err;
    if (!validateInputs(&err))
    {
        setStatus(err, true);
        return;
    }

    emit paramsCommitted(params());

    m_simProgress = 0;
    setCorrecting(true);
    setStatus(tr("正在校正中 0%"));
    m_simTimer->start();
}

void DepthCorrectDialog::onSimulateProgress()
{
    m_simProgress += 5;
    if (m_simProgress > 100)
        m_simProgress = 100;

    setStatus(tr("正在校正中 %1%").arg(m_simProgress));

    if (m_simProgress < 100)
        return;

    m_simTimer->stop();
    setCorrecting(false);

    // TODO: 对接深度矫正后端（或工作流 EXE func_depth_correct）
    setStatus(tr("校正完成"), false);
    emit correctionSucceeded(params());
}
