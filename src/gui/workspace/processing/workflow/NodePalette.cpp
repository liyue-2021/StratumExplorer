#include "NodePalette.h"
#include "WorkflowScene.h" // for kNodeTypeMime

#include "service/processing/INodeFactory.h"
#include "processing/NodeDefinition.h"

#include <QHash>
#include <QMimeData>
#include <algorithm>

using namespace processing;
using namespace processing::gui;

NodePalette::NodePalette(INodeFactory *factory, QWidget *parent)
    : QTreeWidget(parent), m_factory(factory)
{
    setHeaderLabel(tr("节点库"));
    setMinimumWidth(180);

    // 启用拖出（QTreeWidget 默认不允许 drag）
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    setSelectionMode(QAbstractItemView::SingleSelection);

    refresh();

    connect(this, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem *item, int)
            {
                const QString typeId = item->data(0, Qt::UserRole).toString();
                if (!typeId.isEmpty())
                    emit nodeTypeActivated(typeId);
            });
}

void NodePalette::refresh()
{
    clear();
    if (!m_factory)
        return;

    // 获取所有节点并按 order 字段排序，保证显示顺序固定
    auto metas = m_factory->listAll();
    std::sort(metas.begin(), metas.end(),
              [](const NodeMeta &a, const NodeMeta &b)
              {
                  if (a.order != b.order)
                      return a.order < b.order;
                  if (a.group != b.group)
                      return a.group < b.group;
                  if (a.displayName != b.displayName)
                      return a.displayName < b.displayName;
                  return a.typeId < b.typeId;
              });

    QMap<NodeGroup, QTreeWidgetItem *> groupItems; // 改用 QMap 保持有序
    auto getGroup = [&](NodeGroup g) -> QTreeWidgetItem *
    {
        if (auto it = groupItems.constFind(g); it != groupItems.constEnd())
            return *it;
        auto *g_item = new QTreeWidgetItem(this);
        g_item->setText(0, toString(g));
        g_item->setExpanded(true);
        groupItems.insert(g, g_item);
        return g_item;
    };

    for (const auto &m : metas)
    {
        auto *gi = getGroup(m.group);
        auto *it = new QTreeWidgetItem(gi);
        it->setText(0, m.displayName);
        it->setToolTip(0, m.description);
        it->setData(0, Qt::UserRole, m.typeId);
        // 只允许拖动叶子节点（节点类型），分组节点不可拖
        it->setFlags((it->flags() | Qt::ItemIsDragEnabled) & ~Qt::ItemIsDropEnabled);
    }
}

QStringList NodePalette::mimeTypes() const
{
    return {QString::fromLatin1(WorkflowScene::kNodeTypeMime)};
}

QMimeData *NodePalette::mimeData(const QList<QTreeWidgetItem *> &items) const
{
    if (items.isEmpty())
        return nullptr;
    auto *it = items.first();
    const QString typeId = it->data(0, Qt::UserRole).toString();
    if (typeId.isEmpty())
        return nullptr;
    auto *md = new QMimeData;
    md->setData(WorkflowScene::kNodeTypeMime, typeId.toUtf8());
    md->setText(typeId);
    return md;
}
