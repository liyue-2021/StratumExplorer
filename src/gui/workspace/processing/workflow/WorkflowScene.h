#ifndef WORKFLOW_SCENE_H
#define WORKFLOW_SCENE_H

#include <QGraphicsScene>
#include <QHash>
#include <QPointer>

#include "NodeItem.h"

class QGraphicsLineItem;

namespace processing
{
    class IWorkflowEngine;
    class INodeFactory;
    struct NodeInstance;
    struct EdgeInstance;
    struct NodeMeta;
    enum class NodeStatus;
    enum class DataFormat;
}

namespace processing
{
    namespace gui
    {

        class EdgeItem;

        /**
         * @brief DAG 画布场景
         *
         * 监听 IWorkflowEngine 的信号，把节点/边同步到 QGraphicsItem。
         * 同时承担：
         *   - 端口拖拽连线（输出端口拖到下游输入端口）
         *   - 节点右键菜单（运行/重命名/重置参数/删除）
         *   - 节点库拖拽到画布（自定义 mime）
         *   - 节点卡片上 ▶/■ 按钮调用引擎（runSingle / stop）
         *
         * 用户操作通过场景调用引擎接口（不直接改自己状态，等引擎信号回来再更新），
         * 保持单一数据源。
         */
        class WorkflowScene : public QGraphicsScene
        {
            Q_OBJECT
            friend class NodeItem;

        public:
            static constexpr const char *kNodeTypeMime = "application/x-oilpro-nodetype";

            // 构造场景，传入工作流引擎和节点工厂
            explicit WorkflowScene(processing::IWorkflowEngine *engine,
                                   processing::INodeFactory *factory = nullptr,
                                   QObject *parent = nullptr);

            // 获取当前选中节点的唯一标识
            QString selectedNodeId() const;

            // 获取后端工作流引擎和节点工厂指针
            processing::IWorkflowEngine *engine() const { return m_engine; }
            processing::INodeFactory *factory() const { return m_factory; }

            // 请求运行指定的单个节点（供 NodeItem 按钮回调）
            void requestRunNode(const QString &id);
            // 请求停止所有运行中的节点（供 NodeItem 停止按钮）
            void requestStopAll();

            /// 是否绘制甲方序号角标（默认关；序号仍保留在 tooltip）
            void setTestSeqBadgeVisible(bool visible);
            bool testSeqBadgeVisible() const { return m_testSeqBadgeVisible; }

        signals:
            // 用户在画布上双击节点。WorkflowEditorTab 会据此对 display.* 节点
            // 弹出可视化窗口（QCustomPlot）。
            void nodeDoubleClicked(const QString &nodeId);

        protected:
            void mousePressEvent(QGraphicsSceneMouseEvent *e) override;
            void mouseMoveEvent(QGraphicsSceneMouseEvent *e) override;
            void mouseReleaseEvent(QGraphicsSceneMouseEvent *e) override;
            void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e) override;
            void contextMenuEvent(QGraphicsSceneContextMenuEvent *e) override;
            void dragEnterEvent(QGraphicsSceneDragDropEvent *e) override;
            void dragMoveEvent(QGraphicsSceneDragDropEvent *e) override;
            void dropEvent(QGraphicsSceneDragDropEvent *e) override;

        private slots:
            void onNodeAdded(const processing::NodeInstance &n);
            void onNodeRemoved(const QString &id);
            void onNodeRenamed(const QString &id, const QString &newName);
            void onNodeStatusChanged(const QString &id, processing::NodeStatus s);
            void onEdgeAdded(const processing::EdgeInstance &e);
            void onEdgeRemoved(const processing::EdgeInstance &e);

        private:
            NodeItem *nodeItemAt(const QPointF &scenePos) const;
            const processing::NodeMeta *metaOf(const QString &typeId) const;
            QString pickFirstFreeOutputPort(const QString &nodeId, const QString &typeId) const;
            QString pickFirstFreeInputPort(const QString &nodeId, const QString &typeId,
                                           const QString &fromTypeId,
                                           processing::DataFormat fromFormat) const;

            void updateAlignmentGuides(NodeItem *movingNode, const QPointF &proposedPos);
            void applyAlignmentSnap(NodeItem *movingNode);
            void clearAlignmentGuides();

            processing::IWorkflowEngine *m_engine;
            processing::INodeFactory *m_factory;
            QHash<QString, NodeItem *> m_nodeItems;
            QList<EdgeItem *> m_edgeItems;

            // 对齐辅助线状态
            QGraphicsLineItem *m_alignLineV = nullptr;
            QGraphicsLineItem *m_alignLineH = nullptr;
            QPointF m_alignmentSnap;
            bool m_hasAlignmentX = false;
            bool m_hasAlignmentY = false;

            // 拖拽连线临时状态（QPointer 避免节点删除后野指针）
            // FromOutput：从源节点绿色输出口拖出（常规）
            // TowardInput：从目标节点黄色输入口反向寻找上游（便于接「数据输入→格式转换」）
            enum class ConnectDragMode
            {
                None,
                FromOutput,
                TowardInput,
            };
            ConnectDragMode m_connectDrag = ConnectDragMode::None;
            QPointer<NodeItem> m_pendingFrom;
            QPointer<NodeItem> m_pendingTo;
            QGraphicsLineItem *m_previewLine = nullptr;

            bool m_testSeqBadgeVisible = false;
        };

    }
} // namespace processing::gui

#endif // WORKFLOW_SCENE_H
