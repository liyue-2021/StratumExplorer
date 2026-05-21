#include "TimeCorrectDialog.h"
#include "ui_TimeCorrectDialog.h"

#include <QComboBox>
#include <QFileDialog>
#include <QTimer>

using namespace processing::gui;

TimeCorrectDialog::TimeCorrectDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TimeCorrectDialog)
{
    ui->setupUi(this);

    ui->spinTimeOffset->setValue(28800.0);
    ui->spinTimeScale->setValue(1.0);

    m_simTimer = new QTimer(this);
    m_simTimer->setInterval(120);

    setupConnections();
}

TimeCorrectDialog::~TimeCorrectDialog()
{
    delete ui;
}

void TimeCorrectDialog::setupConnections()
{
    connect(ui->btnPickInput, &QPushButton::clicked, this, &TimeCorrectDialog::onPickInput);
    connect(ui->btnPickOutput, &QPushButton::clicked, this, &TimeCorrectDialog::onPickOutput);
    connect(ui->btnCorrect, &QPushButton::clicked, this, &TimeCorrectDialog::onCorrect);
    connect(m_simTimer, &QTimer::timeout, this, &TimeCorrectDialog::onSimulateProgress);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void TimeCorrectDialog::setComboByText(QComboBox *combo, const QString &text) const
{
    if (!combo)
        return;
    const int idx = combo->findText(text);
    if (idx >= 0)
        combo->setCurrentIndex(idx);
}

void TimeCorrectDialog::onPickInput()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("选择文件"), ui->editInputPath->text());
    if (!path.isEmpty())
        ui->editInputPath->setText(path);
}

void TimeCorrectDialog::onPickOutput()
{
    const QString path = QFileDialog::getExistingDirectory(
        this, tr("选择输出文件夹"), ui->editOutputPath->text());
    if (!path.isEmpty())
        ui->editOutputPath->setText(path);
}

void TimeCorrectDialog::setParams(const QVariantMap &params)
{
    ui->editInputPath->setText(params.value(QStringLiteral("input_path")).toString());
    setComboByText(ui->comboFormat,
                   params.value(QStringLiteral("format"), QStringLiteral("OSA by OptaSoft")).toString());
    setComboByText(ui->comboDasDataType,
                   params.value(QStringLiteral("das_data_type"), QStringLiteral("Raw")).toString());
    ui->spinTimeOffset->setValue(params.value(QStringLiteral("time_offset_s"), 28800.0).toDouble());
    ui->spinTimeScale->setValue(params.value(QStringLiteral("time_scale"), 1.0).toDouble());
    setComboByText(ui->comboWriteBack,
                   params.value(QStringLiteral("write_back"), QStringLiteral("Off")).toString());
    setComboByText(ui->comboOutputFormat,
                   params.value(QStringLiteral("output_format"), QStringLiteral(".osa")).toString());
    setComboByText(ui->comboOutputNaming,
                   params.value(QStringLiteral("output_naming"), QStringLiteral("DateTime")).toString());
    ui->editOutputPath->setText(params.value(QStringLiteral("output_path")).toString());
}

QVariantMap TimeCorrectDialog::params() const
{
    QVariantMap p;
    p.insert(QStringLiteral("input_path"), ui->editInputPath->text().trimmed());
    p.insert(QStringLiteral("format"), ui->comboFormat->currentText());
    p.insert(QStringLiteral("das_data_type"), ui->comboDasDataType->currentText());
    p.insert(QStringLiteral("time_offset_s"), ui->spinTimeOffset->value());
    p.insert(QStringLiteral("time_scale"), ui->spinTimeScale->value());
    p.insert(QStringLiteral("write_back"), ui->comboWriteBack->currentText());
    p.insert(QStringLiteral("output_format"), ui->comboOutputFormat->currentText());
    p.insert(QStringLiteral("output_naming"), ui->comboOutputNaming->currentText());
    p.insert(QStringLiteral("output_path"), ui->editOutputPath->text().trimmed());
    return p;
}

bool TimeCorrectDialog::validateInputs(QString *errorMessage) const
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

void TimeCorrectDialog::setCorrecting(bool correcting)
{
    m_correcting = correcting;
    ui->btnCorrect->setEnabled(!correcting);
    ui->btnPickInput->setEnabled(!correcting);
    ui->btnPickOutput->setEnabled(!correcting);
    ui->comboFormat->setEnabled(!correcting);
    ui->comboDasDataType->setEnabled(!correcting);
    ui->spinTimeOffset->setEnabled(!correcting);
    ui->spinTimeScale->setEnabled(!correcting);
    ui->comboWriteBack->setEnabled(!correcting);
    ui->comboOutputFormat->setEnabled(!correcting);
    ui->comboOutputNaming->setEnabled(!correcting);
    ui->editInputPath->setEnabled(!correcting);
    ui->editOutputPath->setEnabled(!correcting);
    ui->btnCorrect->setText(correcting ? tr("正在校正中") : tr("校正"));
}

void TimeCorrectDialog::setStatus(const QString &text, bool isError)
{
    ui->labelStatus->setText(text);
    if (isError)
        ui->labelStatus->setStyleSheet(
            QStringLiteral("color:#C62828; padding: 4px 0; font-weight: bold;"));
    else
        ui->labelStatus->setStyleSheet(QStringLiteral("color:#546E7A; padding: 4px 0;"));
}

void TimeCorrectDialog::onCorrect()
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

void TimeCorrectDialog::onSimulateProgress()
{
    m_simProgress += 5;
    if (m_simProgress > 100)
        m_simProgress = 100;

    setStatus(tr("正在校正中 %1%").arg(m_simProgress));

    if (m_simProgress < 100)
        return;

    m_simTimer->stop();
    setCorrecting(false);

    // TODO: 对接时间矫正后端（func_depth_time_correct）
    setStatus(tr("校正完成"), false);
    emit correctionSucceeded(params());
}
