#include "FormatConvertDialog.h"
#include "ui_FormatConvertDialog.h"

#include <QFileDialog>
#include <QTimer>
#include <QRegularExpression>

using namespace processing::gui;

namespace
{
    constexpr auto kOutputTypeH5 = "H5";
} // namespace

FormatConvertDialog::FormatConvertDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FormatConvertDialog)
{
    ui->setupUi(this);

    m_simTimer = new QTimer(this);
    m_simTimer->setInterval(120);

    setupConnections();
}

FormatConvertDialog::~FormatConvertDialog()
{
    delete ui;
}

void FormatConvertDialog::setupConnections()
{
    connect(ui->btnPickImport, &QPushButton::clicked, this, &FormatConvertDialog::onPickImport);
    connect(ui->btnPickOutput, &QPushButton::clicked, this, &FormatConvertDialog::onPickOutput);
    connect(ui->btnStartConvert, &QPushButton::clicked, this, &FormatConvertDialog::onStartConvert);
    connect(m_simTimer, &QTimer::timeout, this, &FormatConvertDialog::onSimulateProgress);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void FormatConvertDialog::onPickImport()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("选择导入文件"), ui->editImportPath->text());
    if (!path.isEmpty())
        ui->editImportPath->setText(path);
}

void FormatConvertDialog::onPickOutput()
{
    const QString path = QFileDialog::getExistingDirectory(
        this, tr("选择输出目录"), ui->editOutputPath->text());
    if (!path.isEmpty())
        ui->editOutputPath->setText(path);
}

void FormatConvertDialog::setParams(const QVariantMap &params)
{
    ui->editImportPath->setText(params.value(QStringLiteral("import_path")).toString());

    const QString importType = params.value(QStringLiteral("import_type"), QStringLiteral("OSA")).toString();
    const int importIdx = ui->comboImportType->findText(importType);
    if (importIdx >= 0)
        ui->comboImportType->setCurrentIndex(importIdx);

    ui->comboOutputType->setCurrentIndex(0);

    ui->editTimeDelay->setText(params.value(QStringLiteral("time_delay")).toString());
    ui->editNaming->setText(params.value(QStringLiteral("naming_rule")).toString());
    ui->editOutputPath->setText(params.value(QStringLiteral("output_path")).toString());
}

QVariantMap FormatConvertDialog::params() const
{
    QVariantMap p;
    p.insert(QStringLiteral("import_path"), ui->editImportPath->text().trimmed());
    p.insert(QStringLiteral("import_type"), ui->comboImportType->currentText());
    p.insert(QStringLiteral("output_type"), QString::fromLatin1(kOutputTypeH5));
    p.insert(QStringLiteral("time_delay"), ui->editTimeDelay->text().trimmed());
    p.insert(QStringLiteral("naming_rule"), ui->editNaming->text().trimmed());
    p.insert(QStringLiteral("output_path"), ui->editOutputPath->text().trimmed());
    return p;
}

bool FormatConvertDialog::validateInputs(QString *errorMessage) const
{
    if (ui->editImportPath->text().trimmed().isEmpty())
    {
        if (errorMessage)
            *errorMessage = tr("请选择导入文件");
        return false;
    }

    if (ui->editOutputPath->text().trimmed().isEmpty())
    {
        if (errorMessage)
            *errorMessage = tr("请选择输出目录");
        return false;
    }

    const QString delayText = ui->editTimeDelay->text().trimmed();
    if (delayText.isEmpty())
    {
        if (errorMessage)
            *errorMessage = tr("时延不能为空");
        return false;
    }
    static const QRegularExpression numRe(QStringLiteral(R"(^-?\d+(\.\d+)?$)"));
    if (!numRe.match(delayText).hasMatch())
    {
        if (errorMessage)
            *errorMessage = tr("时延必须是数字（整数或小数）");
        return false;
    }

    const QString naming = ui->editNaming->text().trimmed();
    if (naming.isEmpty())
    {
        if (errorMessage)
            *errorMessage = tr("命名不能为空");
        return false;
    }
    static const QRegularExpression illegalRe(QStringLiteral(R"([\\/:*?"<>|])"));
    if (illegalRe.match(naming).hasMatch())
    {
        if (errorMessage)
            *errorMessage = tr(R"(命名不能包含下列字符：\ / : * ? " < > |)");
        return false;
    }

    return true;
}

void FormatConvertDialog::setConverting(bool converting)
{
    m_converting = converting;
    ui->btnStartConvert->setEnabled(!converting);
    ui->btnPickImport->setEnabled(!converting);
    ui->comboImportType->setEnabled(!converting);
    ui->editTimeDelay->setEnabled(!converting);
    ui->editNaming->setEnabled(!converting);
    ui->btnPickOutput->setEnabled(!converting);
    ui->editImportPath->setEnabled(!converting);
    ui->editOutputPath->setEnabled(!converting);

    ui->btnStartConvert->setText(converting ? tr("正在转换中") : tr("开始转换"));
}

void FormatConvertDialog::setStatus(const QString &text, bool isError)
{
    ui->labelStatus->setText(text);
    if (isError)
        ui->labelStatus->setStyleSheet(
            QStringLiteral("color:#C62828; padding: 4px 0; font-weight: bold;"));
    else
        ui->labelStatus->setStyleSheet(QStringLiteral("color:#546E7A; padding: 4px 0;"));
}

void FormatConvertDialog::onStartConvert()
{
    if (m_converting)
        return;

    QString err;
    if (!validateInputs(&err))
    {
        setStatus(err, true);
        return;
    }

    emit paramsCommitted(params());

    m_simProgress = 0;
    setConverting(true);
    setStatus(tr("正在转换中 0%"));
    m_simTimer->start();
}

void FormatConvertDialog::onSimulateProgress()
{
    m_simProgress += 5;
    if (m_simProgress > 100)
        m_simProgress = 100;

    setStatus(tr("正在转换中 %1%").arg(m_simProgress));

    if (m_simProgress < 100)
        return;

    m_simTimer->stop();
    setConverting(false);

    // TODO: 替换为应用内格式转换后端接口；失败时 emit conversionFailed(...)
    setStatus(tr("转换完成"), false);
    emit conversionSucceeded(params());
}
