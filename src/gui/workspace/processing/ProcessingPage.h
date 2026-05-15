#ifndef PROCESSING_Page_H
#define PROCESSING_Page_H

#include <QWidget>
#include <QString>
#include <QList>
// 新增：包含数据操作接口头文件
#include "widgets/IDataOperateInterface.h"

class QStackedWidget;
class QTabWidget;
class QTableWidget;

namespace Ui
{
    class ProcessingPage;
}

namespace processing
{
    class IWorkflowEngine;
    class INodeFactory;
    namespace gui
    {
        class WorkflowEditorTab;
    }
}

// 修改：多继承 QWidget + 数据操作接口
class ProcessingPage : public QWidget, public IDataOperateInterface
{
    Q_OBJECT

public:
    explicit ProcessingPage(QWidget *parent = nullptr);
    ~ProcessingPage();

    bool promptSaveCurrentWorkflowIfDirty(const QString &title, const QString &message);

    // 新增：声明重写接口函数（统一规范）
    void clearAllDataAndDisableBtn() override;
    void loadAllDataAndEnableBtn() override;

private:
    void createPresetPage();
    void addPreset(const QString &name, const QString &creator, const QString &remark, const QByteArray &workflowData);
    void saveCurrentWorkflowAsPreset(const QString &name, const QString &creator, const QString &remark);
    bool saveCurrentWorkflow();
    void onRequestClearCanvas();
    void showPresetPage();
    void showWorkflowEditor(const QString &presetName = {});
    void updatePresetTable();
    void removePreset(int row);
    void editPresetMetadata(int row);
    void openPresetDetails(int row);
    void onSaveWorkflowRequested();
    void loadPresetsFromFile();
    void savePresetsToFile();

    struct PresetItem
    {
        QString name;
        QString creator;
        QString remark;
        QString createdAt;
        QByteArray workflowData;
    };

    Ui::ProcessingPage *ui;

    // 工作流引擎由 ProcessingPage 父对象持有，确保界面销毁前引擎仍有效
    processing::IWorkflowEngine *m_workflowEngine = nullptr;
    QStackedWidget *m_mainStack = nullptr;
    QWidget *m_presetPage = nullptr;
    QTableWidget *m_presetTable = nullptr;
    QTabWidget *m_innerTabs = nullptr;
    processing::gui::WorkflowEditorTab *m_editor = nullptr;
    QList<PresetItem> m_presets;
    QString m_currentPresetName; // 当前加载的预设名称，用于保存时判断是否更新
};

#endif // PROCESSING_Page_H
