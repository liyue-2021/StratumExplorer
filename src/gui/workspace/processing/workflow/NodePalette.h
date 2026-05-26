#ifndef NODE_PALETTE_H
#define NODE_PALETTE_H

#include <QTreeWidget>

namespace processing { class INodeFactory; }

namespace processing { namespace gui {

/**
 * @brief 左侧节点库
 *
 * 从 INodeFactory 拉取所有已注册节点，按 NodeGroup 分组展示。
 * 双击节点 → 发 nodeTypeActivated(typeId) 信号到 Tab。
 */
class NodePalette : public QTreeWidget {
    Q_OBJECT
public:
    explicit NodePalette(processing::INodeFactory* factory, QWidget* parent = nullptr);

    void refresh();

    /// 与画布节点角标共用工具栏开关
    void setTestSeqBadgeVisible(bool visible);
    bool testSeqBadgeVisible() const { return m_testSeqBadgeVisible; }

signals:
    void nodeTypeActivated(const QString& typeId);

protected:
    QStringList mimeTypes() const override;
    QMimeData*  mimeData(const QList<QTreeWidgetItem*>& items) const override;

private:
    void applySeqLabelsToItems();

    processing::INodeFactory* m_factory;
    bool m_testSeqBadgeVisible = false;
};

}} // namespace processing::gui

#endif // NODE_PALETTE_H
