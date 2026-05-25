#include "ProcessingPage.h"
#include "ui_ProcessingPage.h"

#include "workflow/WorkflowEditorTab.h"
#include "WorkflowEngine.h"
#include "NodeRegistry.h"
#include "DemoNodes.h"
#include "ProductionNodes.h"

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
                     const QString &title = {},
                     const QString &name = {},
                     const QString &creator = {},
                     const QString &remark = {})
            : QDialog(parent)
        {
            setWindowTitle(title.isEmpty() ? tr("新建工作流预设") : title);
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

    // 1) 注册真实算法节点（来自甲方节点规格文档）
    processing::production::registerProductionNodes();

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
    connect(m_editor, &processing::gui::WorkflowEditorTab::requestClearCanvas,
            this, &ProcessingPage::onRequestClearCanvas);

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

    m_presetTable = new QTableWidget(0, 7, m_presetPage);
    m_presetTable->setHorizontalHeaderLabels({tr("序号"), tr("名称"), tr("创建时间"), tr("最近修改"), tr("创建人"), tr("备注"), tr("操作")});
    m_presetTable->setStyleSheet(
        QStringLiteral("QTableWidget { background: white; gridline-color: #E0E0E0; border: 1px solid #CFD8DC; "
                       "border-radius: 4px; }"
                       "QTableWidget::item { padding: 4px 6px; }"
                       "QHeaderView::section { background: #ECEFF1; color: #37474F; padding: 6px 8px; "
                       "border: none; border-bottom: 1px solid #B0BEC5; font-weight: bold; }"));
    m_presetTable->horizontalHeader()->setStretchLastSection(false);
    m_presetTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_presetTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_presetTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_presetTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_presetTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_presetTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    m_presetTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
    m_presetTable->setColumnWidth(0, 52);
    m_presetTable->setColumnWidth(6, 200);
    m_presetTable->verticalHeader()->setVisible(false);
    m_presetTable->verticalHeader()->setDefaultSectionSize(34);
    m_presetTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_presetTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_presetTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_presetTable->setAlternatingRowColors(true);
    m_presetTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_presetTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_presetTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_presetTable->setFixedHeight(m_presetTable->horizontalHeader()->height()
                                  + m_presetTable->verticalHeader()->defaultSectionSize() * kPresetsPerPage
                                  + 2);
    layout->addWidget(m_presetTable);

    auto *pagerLayout = new QHBoxLayout();
    pagerLayout->setSpacing(12);
    m_btnPresetPrevPage = new QPushButton(tr("< 上一页"), m_presetPage);
    m_btnPresetNextPage = new QPushButton(tr("下一页 >"), m_presetPage);
    m_btnPresetPrevPage->setFixedWidth(96);
    m_btnPresetNextPage->setFixedWidth(96);
    m_presetPageLabel = new QLabel(m_presetPage);
    m_presetPageLabel->setAlignment(Qt::AlignCenter);
    m_presetPageLabel->setStyleSheet(QStringLiteral("color:#546E7A; font-size:13px;"));
    pagerLayout->addStretch();
    pagerLayout->addWidget(m_btnPresetPrevPage);
    pagerLayout->addWidget(m_presetPageLabel);
    pagerLayout->addWidget(m_btnPresetNextPage);
    pagerLayout->addStretch();
    layout->addLayout(pagerLayout);

    connect(m_btnPresetPrevPage, &QPushButton::clicked, this, [this]()
            { goToPresetPage(m_presetPageIndex - 1); });
    connect(m_btnPresetNextPage, &QPushButton::clicked, this, [this]()
            { goToPresetPage(m_presetPageIndex + 1); });

    auto *hint = new QLabel(tr("单击“详情”进入工作流；“编辑”修改名称、创建人与备注。每页显示 15 条。"), m_presetPage);
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
}

void ProcessingPage::addPreset(const QString &name, const QString &creator, const QString &remark, const QByteArray &workflowData)
{
    PresetItem item;
    item.name = name;
    item.creator = creator;
    item.remark = remark;
    QString now = QDateTime::currentDateTime().toString("yyyy/M/d HH:mm");
    item.createdAt = now;
    item.modifiedAt = now;
    item.workflowData = workflowData;
    m_presets.append(item);
    m_presetPageIndex = totalPresetPages() - 1;
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

bool ProcessingPage::saveCurrentWorkflow()
{
    if (!m_editor)
        return false;

    const QByteArray workflowData = m_editor->currentWorkflowData();
    if (workflowData.isEmpty())
    {
        QMessageBox::warning(this, tr("保存失败"), tr("当前工作流数据为空，无法保存。"));
        return false;
    }

    bool hasCurrentPreset = !m_currentPresetName.isEmpty();
    SaveWorkflowDialog saveDialog(this, hasCurrentPreset);
    if (saveDialog.exec() != QDialog::Accepted)
        return false;

    if (hasCurrentPreset && saveDialog.updateCurrent())
    {
        // 更新当前预设
        for (int i = 0; i < m_presets.size(); ++i)
        {
            if (m_presets[i].name == m_currentPresetName)
            {
                m_presets[i].workflowData = workflowData;
                m_presets[i].modifiedAt = QDateTime::currentDateTime().toString("yyyy/M/d HH:mm");
                updatePresetTable();
                savePresetsToFile();
                QMessageBox::information(this, tr("保存成功"), tr("已更新预设：%1").arg(m_currentPresetName));
                if (m_editor)
                    m_editor->updateSavedWorkflowSnapshot();
                return true;
            }
        }
        // 如果找不到当前预设，fallback 到另存
    }

    // 另存为新预设
    PresetDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted)
    {
        saveCurrentWorkflowAsPreset(dlg.presetName(), dlg.creator(), dlg.remark());
        if (m_editor)
            m_editor->updateSavedWorkflowSnapshot();
        return true;
    }

    return false;
}

bool ProcessingPage::promptSaveCurrentWorkflowIfDirty(const QString &title, const QString &message)
{
    if (!m_editor || !m_editor->hasUnsavedChanges())
        return true;

    const QMessageBox::StandardButton result = QMessageBox::question(
        this,
        title,
        message,
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
        QMessageBox::Yes);

    if (result == QMessageBox::Yes)
        return saveCurrentWorkflow();
    if (result == QMessageBox::No)
        return true;
    return false;
}

void ProcessingPage::onSaveWorkflowRequested()
{
    saveCurrentWorkflow();
}

int ProcessingPage::totalPresetPages() const
{
    if (m_presets.isEmpty())
        return 1;
    return (m_presets.size() + kPresetsPerPage - 1) / kPresetsPerPage;
}

void ProcessingPage::goToPresetPage(int pageIndex)
{
    const int pages = totalPresetPages();
    m_presetPageIndex = qBound(0, pageIndex, pages - 1);
    updatePresetTable();
}

void ProcessingPage::updatePaginationBar()
{
    const int total = m_presets.size();
    const int pages = totalPresetPages();
    const int current = m_presetPageIndex + 1;

    if (m_presetPageLabel)
    {
        m_presetPageLabel->setText(
            total == 0
                ? tr("暂无预设")
                : tr("第 %1 / %2 页，共 %3 条").arg(current).arg(pages).arg(total));
    }
    if (m_btnPresetPrevPage)
        m_btnPresetPrevPage->setEnabled(m_presetPageIndex > 0);
    if (m_btnPresetNextPage)
        m_btnPresetNextPage->setEnabled(m_presetPageIndex < pages - 1);
}

void ProcessingPage::updatePresetTable()
{
    if (!m_presetTable)
        return;

    const int total = m_presets.size();
    const int pages = totalPresetPages();
    if (m_presetPageIndex >= pages)
        m_presetPageIndex = qMax(0, pages - 1);

    const int start = m_presetPageIndex * kPresetsPerPage;
    const int end = qMin(start + kPresetsPerPage, total);
    const int rowsOnPage = end - start;

    m_presetTable->setRowCount(rowsOnPage);
    for (int i = 0; i < rowsOnPage; ++i)
    {
        const int globalRow = start + i;
        const auto &item = m_presets.at(globalRow);
        m_presetTable->setItem(i, 0, new QTableWidgetItem(QString::number(globalRow + 1)));
        m_presetTable->setItem(i, 1, new QTableWidgetItem(item.name));
        m_presetTable->setItem(i, 2, new QTableWidgetItem(item.createdAt));
        m_presetTable->setItem(i, 3, new QTableWidgetItem(item.modifiedAt));
        m_presetTable->setItem(i, 4, new QTableWidgetItem(item.creator));
        auto *remarkItem = new QTableWidgetItem(item.remark);
        remarkItem->setToolTip(item.remark);
        m_presetTable->setItem(i, 5, remarkItem);

        auto *ops = new QWidget(m_presetTable);
        auto *opsLayout = new QHBoxLayout(ops);
        opsLayout->setContentsMargins(4, 2, 4, 2);
        opsLayout->setSpacing(6);
        auto *editBtn = new QPushButton(tr("编辑"), ops);
        auto *detailBtn = new QPushButton(tr("详情"), ops);
        auto *deleteBtn = new QPushButton(tr("删除"), ops);
        editBtn->setFixedHeight(26);
        detailBtn->setFixedHeight(26);
        deleteBtn->setFixedHeight(26);
        detailBtn->setStyleSheet(QStringLiteral("QPushButton { color: #1565C0; font-weight: bold; }"));
        opsLayout->addWidget(editBtn);
        opsLayout->addWidget(detailBtn);
        opsLayout->addWidget(deleteBtn);
        opsLayout->addStretch();
        m_presetTable->setCellWidget(i, 6, ops);

        connect(editBtn, &QPushButton::clicked, this, [this, globalRow]()
                { editPresetMetadata(globalRow); });
        connect(detailBtn, &QPushButton::clicked, this, [this, globalRow]()
                { openPresetDetails(globalRow); });
        connect(deleteBtn, &QPushButton::clicked, this, [this, globalRow]()
                { removePreset(globalRow); });
    }

    adjustPresetColumnWidths();
    updatePaginationBar();
}

void ProcessingPage::adjustPresetColumnWidths()
{
    if (!m_presetTable || m_presetTable->rowCount() == 0)
        return;

    constexpr int kNameMaxWidth = 200;
    m_presetTable->resizeColumnToContents(1);
    if (m_presetTable->columnWidth(1) > kNameMaxWidth)
        m_presetTable->setColumnWidth(1, kNameMaxWidth);

    m_presetTable->resizeColumnToContents(4);
    constexpr int kCreatorMaxWidth = 120;
    if (m_presetTable->columnWidth(4) > kCreatorMaxWidth)
        m_presetTable->setColumnWidth(4, kCreatorMaxWidth);
}

void ProcessingPage::removePreset(int globalRow)
{
    if (globalRow < 0 || globalRow >= m_presets.size())
        return;

    const auto reply = QMessageBox::question(
        this,
        tr("删除预设"),
        tr("确定删除预设「%1」吗？").arg(m_presets.at(globalRow).name),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (reply != QMessageBox::Yes)
        return;

    m_presets.removeAt(globalRow);

    const int pages = totalPresetPages();
    if (m_presetPageIndex >= pages)
        m_presetPageIndex = qMax(0, pages - 1);

    updatePresetTable();
    savePresetsToFile();
}

void ProcessingPage::editPresetMetadata(int globalRow)
{
    if (globalRow < 0 || globalRow >= m_presets.size())
        return;

    auto item = m_presets.at(globalRow);
    PresetDialog dlg(this, tr("编辑预设信息"), item.name, item.creator, item.remark);
    if (dlg.exec() != QDialog::Accepted)
        return;

    item.name = dlg.presetName();
    item.creator = dlg.creator();
    item.remark = dlg.remark();
    item.modifiedAt = QDateTime::currentDateTime().toString("yyyy/M/d HH:mm");
    m_presets[globalRow] = item;
    updatePresetTable();
    savePresetsToFile();
}

void ProcessingPage::openPresetDetails(int globalRow)
{
    if (globalRow < 0 || globalRow >= m_presets.size() || !m_editor)
        return;

    const auto item = m_presets.at(globalRow);
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
    if (!promptSaveCurrentWorkflowIfDirty(
            tr("保存当前工作区更改"),
            tr("当前工作区有未保存更改，是否保存后切换到预设列表？")))
    {
        return;
    }

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

void ProcessingPage::onRequestClearCanvas()
{
    if (!m_editor)
        return;

    if (!m_editor->hasUnsavedChanges())
    {
        if (m_workflowEngine)
            m_workflowEngine->clear();
        m_currentPresetName.clear();
        if (m_editor)
            m_editor->updateSavedWorkflowSnapshot();
        showWorkflowEditor({});
        return;
    }

    const QMessageBox::StandardButton result = QMessageBox::question(
        this,
        tr("确认清空画布"),
        tr("当前工作流已有更改，是否确认丢弃所有更改并清空画布？"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (result == QMessageBox::Yes)
    {
        if (m_workflowEngine)
            m_workflowEngine->clear();
        m_currentPresetName.clear();
        if (m_editor)
            m_editor->updateSavedWorkflowSnapshot();
        showWorkflowEditor({});
    }
    else
    {
        QMessageBox::information(this, tr("请先保存"), tr("请先保存当前工作区更改或继续编辑。"));
    }
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
        item.modifiedAt = obj["modifiedAt"].toString();
        if (item.modifiedAt.isEmpty())
            item.modifiedAt = item.createdAt; // 兼容旧数据
        item.workflowData = QByteArray::fromBase64(obj["workflowData"].toString().toUtf8());
        m_presets.append(item);
    }
    m_presetPageIndex = 0;
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
        obj["modifiedAt"] = item.modifiedAt;
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
