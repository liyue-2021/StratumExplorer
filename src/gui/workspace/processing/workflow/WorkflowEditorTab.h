#ifndef WORKFLOW_EDITOR_TAB_H
#define WORKFLOW_EDITOR_TAB_H

#include <QWidget>
#include <QPointer>
#include <QPointer>
#include <memory>

namespace processing
{
    class IWorkflowEngine;
    class INodeFactory;
}

class QToolBar;
class QSplitter;
class QListWidget;
class QStatusBar;
class QLabel;
class QFormLayout;
class QScrollArea;

namespace processing
{
    namespace gui
    {

        class WorkflowScene;
        class WorkflowView;
        class NodePalette;

        /**
         * @brief 预处理工作流编辑器（顶层 Tab 页）
         *
         * 布局：
         *   [上]  ToolBar：新建/打开/保存 + 单跑/全跑/停止 + 缩放
         *   [中]  QSplitter
         *           ├── 左：NodePalette  （节点库，按 group 分类）
         *           └── 中右：WorkflowView (画布) + 右侧属性面板
         *   [下]  状态栏（校验失败用红字提示）
         *
         * 本类只负责 UI 装配 + 信号转发，业务逻辑全在 IWorkflowEngine。
         */
        class WorkflowEditorTab : public QWidget
        {
            Q_OBJECT
        public:
            explicit WorkflowEditorTab(processing::IWorkflowEngine *engine,
                                       processing::INodeFactory *factory,
                                       QWidget *parent = nullptr);
            ~WorkflowEditorTab() override;

            QByteArray currentWorkflowData() const;
            bool loadWorkflowData(const QByteArray &data);

        signals:
            void requestClearCanvas();
            void requestOpenSavedWorkflow();
            void requestSaveWorkflow();

        public:
            bool hasUnsavedChanges() const;
            void updateSavedWorkflowSnapshot();

        private slots:
            // 清空画布（由上层处理未保存提示、清空等逻辑）
            void onActionClearCanvas();
            // 打开已保存的工作流列表界面
            void onActionOpenProject();
            // 将当前工作流保存到预设列表
            void onActionSaveProject();
            // 运行当前选中的单个节点
            void onActionRunSingle();
            // 执行整个工作流（所有节点按拓扑顺序运行）
            void onActionRunAll();
            // 停止所有正在运行的节点
            void onActionStop();

            // 节点库中拖入或双击节点时，添加到画布
            void onPaletteDoubleClicked(const QString &typeId);
            // 画布上选中节点时，重建右侧属性面板表单
            void onSelectionChanged();
            // 双击节点（当前仅提示，配置/展示弹窗待甲方确认后恢复）
            void onNodeDoubleClicked(const QString &nodeId);
            void rebuildDataInputParamForm(const QString &nodeId, const QVariantMap &displayParams);
            void rebuildFormatConvertParamForm(const QString &nodeId, const QVariantMap &displayParams);
            void onExportFormatConvert(const QString &nodeId);

            // 状态栏临时提示
            void showStatus(const QString &text, bool error = false, int timeoutMs = 5000);

        private:
            void setupUi();
            void setupConnections();
            void rebuildParamForm(const QString &nodeId);

            // 使用 QPointer 追踪这些对象的有效性，它们可能在 WorkflowEditorTab 前被销毁
            QPointer<processing::IWorkflowEngine> m_engine;
            processing::INodeFactory *m_factory;

            QToolBar *m_toolbar = nullptr;
            QSplitter *m_splitter = nullptr;
            NodePalette *m_palette = nullptr;
            WorkflowView *m_view = nullptr;
            QPointer<WorkflowScene> m_scene = nullptr;
            QWidget *m_paramsPanel = nullptr;
            QFormLayout *m_paramsForm = nullptr;
            QLabel *m_statusLabel = nullptr;
            class LogDock *m_logDock = nullptr;

            QByteArray m_savedWorkflowData;

            // 当前文件路径（“保存”不弹窗、直接覆盖）
            QString m_currentFilePath;
            bool m_currentIsTemplate = true;

            // rebuildParamForm 内会 setNodeParams；防止 nodeParamsChanged 重入导致表单重复
            bool m_rebuildingParamForm = false;
        };

    }
} // namespace processing::gui

#endif // WORKFLOW_EDITOR_TAB_H
