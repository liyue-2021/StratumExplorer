#include "ProcessingPage.h"
#include "ui_ProcessingPage.h"

#include "workflow/WorkflowEditorTab.h"
#include "WorkflowEngine.h"
#include "NodeRegistry.h"
#include "DemoNodes.h"

#include <QTabWidget>
#include <QStackedWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QDateTime>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QSizePolicy>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QRadioButton>
#include <QButtonGroup>

namespace
{
    class PresetDialog : public QDialog
    {
    public:
        PresetDialog(QWidget *parent = nullptr,
                     const QString &name = {},
                     const QString &creator = {},
                     const QString &remark = {})
            : QDialog(parent)
        {
            setWindowTitle(tr("新建工作流预设"));
            setModal(true);
            auto *layout = new QVBoxLayout(this);
            auto *form = new QFormLayout();
            m_name = new QLineEdit(name, this);
            m_creator = new QLineEdit(creator, this);
            m_remark = new QTextEdit(remark, this);
            m_remark->setFixedHeight(90);
            form->addRow(tr("预设名称"), m_name);
            form->addRow(tr("创建人"), m_creator);
            form->addRow(tr("备注"), m_remark);
            layout->addLayout(form);

            auto *buttonBox = new QDialogButtonBox(
                QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                Qt::Horizontal, this);
            connect(buttonBox, &QDialogButtonBox::accepted, this, &PresetDialog::accept);
            connect(buttonBox, &QDialogButtonBox::rejected, this, &PresetDialog::reject);
            layout->addWidget(buttonBox);
        }

        QString presetName() const { return m_name->text().trimmed(); }
        QString creator() const { return m_creator->text().trimmed(); }
        QString remark() const { return m_remark->toPlainText().trimmed(); }

        void accept() override
        {
            if (presetName().isEmpty())
            {
                QMessageBox::warning(this, tr("输入错误"), tr("预设名称不能为空。"));
                return;
            }
            if (creator().isEmpty())
            {
                QMessageBox::warning(this, tr("输入错误"), tr("创建人不能为空。"));
                return;
            }
            QDialog::accept();
        }

    private:
        QLineEdit *m_name;
        QLineEdit *m_creator;
        QTextEdit *m_remark;
    };

    class SaveWorkflowDialog : public QDialog
    {
    public:
        SaveWorkflowDialog(QWidget *parent = nullptr, bool hasCurrentPreset = false)
            : QDialog(parent)
        {
            setWindowTitle(tr("保存工作流"));
            setModal(true);
            auto *layout = new QVBoxLayout(this);

            m_buttonGroup = new QButtonGroup(this);
            if (hasCurrentPreset)
            {
                m_updateCurrent = new QRadioButton(tr("更新当前预设"), this);
                m_buttonGroup->addButton(m_updateCurrent);
                layout->addWidget(m_updateCurrent);
            }
            m_saveAsNew = new QRadioButton(tr("另存为新预设"), this);
            m_buttonGroup->addButton(m_saveAsNew);
            layout->addWidget(m_saveAsNew);

            if (hasCurrentPreset)
                m_updateCurrent->setChecked(true);
            else
                m_saveAsNew->setChecked(true);

            auto *buttonBox = new QDialogButtonBox(
                QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                Qt::Horizontal, this);
            connect(buttonBox, &QDialogButtonBox::accepted, this, &SaveWorkflowDialog::accept);
            connect(buttonBox, &QDialogButtonBox::rejected, this, &SaveWorkflowDialog::reject);
            layout->addWidget(buttonBox);
        }

        bool updateCurrent() const { return m_updateCurrent && m_updateCurrent->isChecked(); }
        bool saveAsNew() const { return m_saveAsNew->isChecked(); }

    private:
        QButtonGroup *m_buttonGroup;
        QRadioButton *m_updateCurrent = nullptr;
        QRadioButton *m_saveAsNew;
    };
}

ProcessingPage::ProcessingPage(QWidget *parent)
    : QWidget(parent), ui(new Ui::ProcessingPage)
{
    ui->setupUi(this);

    // 0) 注册 demo 节点（幂等，重复调也安全）
    processing::demo::registerDemoNodes();

    // 1) 创建工作流引擎实例（注入节点工厂单例）
    auto *factory = &processing::NodeRegistry::instance();
    // 将 engine 归属到 ProcessingPage，以保证它在页面销毁前不会先于 UI 元素析构。
    m_workflowEngine = new processing::WorkflowEngine(factory, this);

    m_mainStack = new QStackedWidget(this);
    createPresetPage();
    m_mainStack->addWidget(m_presetPage);

    // 2) 内嵌一个 QTabWidget，第一个 Tab 就是「工作流编辑器」
    //    后续可以在这里加更多 Tab（结果浏览、批处理等）
    m_innerTabs = new QTabWidget(this);
    m_editor = new processing::gui::WorkflowEditorTab(
        m_workflowEngine, factory, m_innerTabs);
    m_innerTabs->addTab(m_editor, tr("工作流编排"));

    connect(m_editor, &processing::gui::WorkflowEditorTab::requestOpenSavedWorkflow,
            this, &ProcessingPage::showPresetPage);
    connect(m_editor, &processing::gui::WorkflowEditorTab::requestSaveWorkflow,
            this, &ProcessingPage::onSaveWorkflowRequested);

    // 3) 占位页（后续可移除）— 让用户看出还能扩展
    auto *placeholder = new QWidget(m_innerTabs);
    m_innerTabs->addTab(placeholder, tr("结果浏览"));

    m_mainStack->addWidget(m_innerTabs);
    m_mainStack->setCurrentWidget(m_presetPage);

    // 4) 用布局把主栈撑满整个 ProcessingPage
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    lay->addWidget(m_mainStack);

    // 5) 加载预设数据
    loadPresetsFromFile();
}

ProcessingPage::~ProcessingPage()
{
    delete ui;
}

void ProcessingPage::createPresetPage()
{
    m_presetPage = new QWidget(this);
    auto *layout = new QVBoxLayout(m_presetPage);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    auto *headerLayout = new QHBoxLayout();
    auto *title = new QLabel(tr("预设工作流"), m_presetPage);
    title->setStyleSheet("font-size:16px; font-weight:bold;");
    auto *btnNew = new QPushButton(tr("新建预设"), m_presetPage);
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(btnNew);
    layout->addLayout(headerLayout);

    m_presetTable = new QTableWidget(0, 6, m_presetPage);
    m_presetTable->setHorizontalHeaderLabels({tr("序号"), tr("名称"), tr("创建时间"), tr("创建人"), tr("备注"), tr("操作")});
    m_presetTable->horizontalHeader()->setStretchLastSection(true);
    m_presetTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_presetTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_presetTable->verticalHeader()->setVisible(false);
    m_presetTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_presetTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_presetTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_presetTable->setAlternatingRowColors(true);
    m_presetTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_presetTable->setMinimumHeight(220);
    m_presetTable->setMaximumHeight(380);
    layout->addWidget(m_presetTable);

    auto *hint = new QLabel(tr("双击行或点击“编辑”进入该预设工作流。保存后会直接写入预设列表。"), m_presetPage);
    hint->setStyleSheet("color:#607D8B; font-size:12px;");
    layout->addWidget(hint);
    layout->addStretch();

    connect(btnNew, &QPushButton::clicked, this, [this]()
            {
        PresetDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted)
        {
            saveCurrentWorkflowAsPreset(dlg.presetName(), dlg.creator(), dlg.remark());
        } });
    connect(m_presetTable, &QTableWidget::cellDoubleClicked, this,
            [this](int row, int)
            {
                if (row >= 0 && row < m_presets.size())
                {
                    editPreset(row);
                }
            });
}

void ProcessingPage::addPreset(const QString &name, const QString &creator, const QString &remark, const QByteArray &workflowData)
{
    PresetItem item;
    item.name = name;
    item.creator = creator;
    item.remark = remark;
    item.createdAt = QDateTime::currentDateTime().toString("yyyy/M/d HH:mm");
    item.workflowData = workflowData;
    m_presets.append(item);
    updatePresetTable();
    savePresetsToFile();
    showWorkflowEditor(name);
}

void ProcessingPage::saveCurrentWorkflowAsPreset(const QString &name, const QString &creator, const QString &remark)
{
    if (!m_editor)
        return;

    const QByteArray workflowData = m_editor->currentWorkflowData();
    if (workflowData.isEmpty())
    {
        QMessageBox::warning(this, tr("保存失败"), tr("当前工作流数据为空，无法保存到预设。"));
        return;
    }

    addPreset(name, creator, remark, workflowData);
    QMessageBox::information(this, tr("保存成功"), tr("工作流已保存到预设列表。"));
}

void ProcessingPage::onSaveWorkflowRequested()
{
    if (!m_editor)
        return;

    const QByteArray workflowData = m_editor->currentWorkflowData();
    if (workflowData.isEmpty())
    {
        QMessageBox::warning(this, tr("保存失败"), tr("当前工作流数据为空，无法保存。"));
        return;
    }

    bool hasCurrentPreset = !m_currentPresetName.isEmpty();
    SaveWorkflowDialog saveDialog(this, hasCurrentPreset);
    if (saveDialog.exec() != QDialog::Accepted)
        return;

    if (hasCurrentPreset && saveDialog.updateCurrent())
    {
        // 更新当前预设
        for (int i = 0; i < m_presets.size(); ++i)
        {
            if (m_presets[i].name == m_currentPresetName)
            {
                m_presets[i].workflowData = workflowData;
                updatePresetTable();
                savePresetsToFile();
                QMessageBox::information(this, tr("保存成功"), tr("已更新预设：%1").arg(m_currentPresetName));
                return;
            }
        }
        // 如果找不到当前预设，fallback 到另存
    }

    // 另存为新预设
    PresetDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted)
    {
        saveCurrentWorkflowAsPreset(dlg.presetName(), dlg.creator(), dlg.remark());
    }
}

void ProcessingPage::updatePresetTable()
{
    m_presetTable->setRowCount(m_presets.size());
    for (int row = 0; row < m_presets.size(); ++row)
    {
        const auto &item = m_presets.at(row);
        m_presetTable->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
        m_presetTable->setItem(row, 1, new QTableWidgetItem(item.name));
        m_presetTable->setItem(row, 2, new QTableWidgetItem(item.createdAt));
        m_presetTable->setItem(row, 3, new QTableWidgetItem(item.creator));
        m_presetTable->setItem(row, 4, new QTableWidgetItem(item.remark));

        auto *ops = new QWidget(m_presetTable);
        auto *opsLayout = new QHBoxLayout(ops);
        opsLayout->setContentsMargins(0, 0, 0, 0);
        opsLayout->setSpacing(6);
        auto *editBtn = new QPushButton(tr("编辑"), ops);
        auto *deleteBtn = new QPushButton(tr("删除"), ops);
        editBtn->setFixedHeight(24);
        deleteBtn->setFixedHeight(24);
        opsLayout->addWidget(editBtn);
        opsLayout->addWidget(deleteBtn);
        opsLayout->addStretch();
        m_presetTable->setCellWidget(row, 5, ops);

        connect(editBtn, &QPushButton::clicked, this, [this, row]()
                { editPreset(row); });
        connect(deleteBtn, &QPushButton::clicked, this, [this, row]()
                { removePreset(row); });
    }
}

void ProcessingPage::removePreset(int row)
{
    if (row < 0 || row >= m_presets.size())
        return;
    m_presets.removeAt(row);
    updatePresetTable();
    savePresetsToFile();
}

void ProcessingPage::editPreset(int row)
{
    if (row < 0 || row >= m_presets.size() || !m_editor)
        return;

    const auto item = m_presets.at(row);
    if (item.workflowData.isEmpty())
    {
        QMessageBox::warning(this, tr("打开失败"), tr("所选预设没有可加载的工作流数据。"));
        return;
    }

    if (!m_editor->loadWorkflowData(item.workflowData))
    {
        QMessageBox::warning(this, tr("打开失败"), tr("加载预设工作流失败，请检查数据格式。"));
        return;
    }

    showWorkflowEditor(item.name);
}
void ProcessingPage::showPresetPage()
{
    if (m_mainStack)
        m_mainStack->setCurrentWidget(m_presetPage);
}

void ProcessingPage::showWorkflowEditor(const QString &presetName)
{
    if (!m_mainStack || !m_innerTabs || !m_editor)
        return;
    m_mainStack->setCurrentWidget(m_innerTabs);
    const int index = m_innerTabs->indexOf(m_editor);
    if (index >= 0)
    {
        const QString title = presetName.isEmpty()
                                  ? tr("工作流编排")
                                  : tr("工作流编排 - %1").arg(presetName);
        m_innerTabs->setTabText(index, title);
    }
    m_currentPresetName = presetName; // 设置当前预设名称
}

// 新增：接口函数空实现（统一规范，后续填充业务逻辑）
void ProcessingPage::clearAllDataAndDisableBtn()
{
    if (m_workflowEngine)
        m_workflowEngine->clear();
}

void ProcessingPage::loadAllDataAndEnableBtn()
{
    // 切换项目时由这里加载工作流模板（后续接 DAO）
}

void ProcessingPage::loadPresetsFromFile()
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);
    if (!dir.exists())
        dir.mkpath(appDataPath);
    QString filePath = appDataPath + "/presets.json";

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return; // 文件不存在或无法打开，忽略

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray())
        return;

    QJsonArray array = doc.array();
    m_presets.clear();
    for (const QJsonValue &value : array)
    {
        QJsonObject obj = value.toObject();
        PresetItem item;
        item.name = obj["name"].toString();
        item.creator = obj["creator"].toString();
        item.remark = obj["remark"].toString();
        item.createdAt = obj["createdAt"].toString();
        item.workflowData = QByteArray::fromBase64(obj["workflowData"].toString().toUtf8());
        m_presets.append(item);
    }
    updatePresetTable();
}

void ProcessingPage::savePresetsToFile()
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);
    if (!dir.exists())
        dir.mkpath(appDataPath);
    QString filePath = appDataPath + "/presets.json";

    QJsonArray array;
    for (const PresetItem &item : m_presets)
    {
        QJsonObject obj;
        obj["name"] = item.name;
        obj["creator"] = item.creator;
        obj["remark"] = item.remark;
        obj["createdAt"] = item.createdAt;
        obj["workflowData"] = QString::fromUtf8(item.workflowData.toBase64());
        array.append(obj);
    }

    QJsonDocument doc(array);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(doc.toJson());
    }
}
