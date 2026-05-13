#include "WorkflowEditorTab.h"
#include "WorkflowScene.h"
#include "PlotDialog.h"
#include "LogDock.h"
#include "WorkflowView.h"
#include "NodePalette.h"
#include "EdgeItem.h"

#include "service/processing/IWorkflowEngine.h"
#include "service/processing/INodeFactory.h"

#include <QToolBar>
#include <QAction>
#include <QSplitter>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTimer>
#include <QDebug>
#include <QScrollArea>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLayoutItem>
#include <QToolButton>

using namespace processing;
using namespace processing::gui;

WorkflowEditorTab::WorkflowEditorTab(IWorkflowEngine *engine,
                                     INodeFactory *factory,
                                     QWidget *parent)
    : QWidget(parent), m_engine(engine), m_factory(factory)
{
    setupUi();
    setupConnections();
}

WorkflowEditorTab::~WorkflowEditorTab()
{
    // 销毁顺序很关键：由于 ProcessingPage 先创建 m_workflowEngine，
    // 它可能会先于 WorkflowEditorTab 被销毁。使用 QPointer 检查有效性。

    // 1. 先检查 scene 是否仍有效，如果有则清空
    if (m_scene)
        m_scene->clear();

    // 2. 检查 engine 是否仍有效，如果有则停止
    if (m_engine)
        m_engine->stop();

    // 3. 断开与 m_scene/m_engine 的连接（如果仍有效）
    if (m_scene)
        disconnect(m_scene, nullptr, this, nullptr);
    if (m_engine)
        disconnect(m_engine, nullptr, this, nullptr);

    // 4. 断开所有其他与 this 的连接（工具栏等）
    disconnect(this);
}

void WorkflowEditorTab::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ---- 顶部工具栏 ----
    m_toolbar = new QToolBar(this);
    m_toolbar->setIconSize(QSize(18, 18));
    m_toolbar->addAction(tr("新建"), this, &WorkflowEditorTab::onActionNew);
    m_toolbar->addAction(tr("打开工作流"), this, &WorkflowEditorTab::onActionOpenProject);
    m_toolbar->addAction(tr("保存工作流"), this, &WorkflowEditorTab::onActionSaveProject);
    m_toolbar->addSeparator();
    m_toolbar->addAction(tr("运行选中节点"), this, &WorkflowEditorTab::onActionRunSingle);
    m_toolbar->addAction(tr("全部运行"), this, &WorkflowEditorTab::onActionRunAll);
    m_toolbar->addAction(tr("停止"), this, &WorkflowEditorTab::onActionStop);
    m_toolbar->addSeparator();
    // 缩放（使用纯文字按钮，等以后接图标 .qrc）
    auto *aZoomIn = m_toolbar->addAction(tr("放大 (Ctrl++)"));
    auto *aZoomOut = m_toolbar->addAction(tr("缩小 (Ctrl+-)"));
    auto *aZoomFit = m_toolbar->addAction(tr("适应 (Ctrl+F)"));
    auto *aZoomRst = m_toolbar->addAction(tr("100% (Ctrl+0)"));
    m_toolbar->addSeparator();
    // 连线风格切换（曲线 / 折线）
    auto *aEdgeStyle = m_toolbar->addAction(tr("连线：曲线"));
    aEdgeStyle->setCheckable(true);
    aEdgeStyle->setChecked(EdgeItem::style() == EdgeItem::Style::Bezier);
    aEdgeStyle->setToolTip(tr("点击切换为折线 / 曲线"));
    connect(aEdgeStyle, &QAction::toggled, this, [this, aEdgeStyle](bool curve)
            {
        EdgeItem::setStyle(curve ? EdgeItem::Style::Bezier
                                 : EdgeItem::Style::Orthogonal);
        aEdgeStyle->setText(curve ? tr("连线：曲线")\
                                  : tr("连线：折线"));
        if (m_scene) m_scene->update(); });
    if (auto *btn = qobject_cast<QToolButton *>(m_toolbar->widgetForAction(aEdgeStyle)))
    {
        btn->setStyleSheet("QToolButton { color: #37474F; background: transparent; }"
                           "QToolButton:checked { background: rgba(55, 71, 79, 0.1); }");
    }
    root->addWidget(m_toolbar);

    // ---- 中央：左 节点库 / 中 画布 / 右 参数面板 ----
    m_splitter = new QSplitter(Qt::Horizontal, this);

    m_palette = new NodePalette(m_factory, m_splitter);

    m_scene = new WorkflowScene(m_engine, m_factory, m_splitter);
    m_view = new WorkflowView(m_scene, m_splitter);

    auto *right = new QWidget(m_splitter);
    auto *rl = new QVBoxLayout(right);
    rl->setContentsMargins(6, 6, 6, 6);
    auto *lblParam = new QLabel(tr("属性面板"), right);
    QFont f = lblParam->font();
    f.setBold(true);
    lblParam->setFont(f);
    rl->addWidget(lblParam);

    // 用 QScrollArea 承载参数表单（节点参数可能很多）
    auto *scroll = new QScrollArea(right);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    m_paramsPanel = new QWidget(scroll);
    m_paramsForm = new QFormLayout(m_paramsPanel);
    m_paramsForm->setContentsMargins(4, 4, 4, 4);
    m_paramsForm->setLabelAlignment(Qt::AlignRight);
    m_paramsForm->addRow(new QLabel(tr("（未选中节点）"), m_paramsPanel));
    scroll->setWidget(m_paramsPanel);
    rl->addWidget(scroll, 1);

    m_splitter->addWidget(m_palette);
    m_splitter->addWidget(m_view);
    m_splitter->addWidget(right);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setStretchFactor(2, 0);
    m_splitter->setSizes({200, 800, 240});

    // ---- 底部日志面板（监听全局 LogBus）----
    // 日志面板暂时隐藏，只注释界面布局，后端日志实现仍保留
    // m_logDock = new processing::gui::LogDock(this);
    // auto *vsplit = new QSplitter(Qt::Vertical, this);
    // vsplit->addWidget(m_splitter);
    // vsplit->addWidget(m_logDock);
    // vsplit->setStretchFactor(0, 1);
    // vsplit->setStretchFactor(1, 0);
    // vsplit->setSizes({600, 160});
    // root->addWidget(vsplit, 1);
    root->addWidget(m_splitter, 1);

    // ---- 底部状态栏（自定义 QLabel，校验失败用红字）----
    m_statusLabel = new QLabel(tr("就绪"), this);
    m_statusLabel->setContentsMargins(8, 4, 8, 4);
    m_statusLabel->setStyleSheet("background:#ECEFF1; color:#37474F;");
    root->addWidget(m_statusLabel);

    // 缩放按钮接通
    connect(aZoomIn, &QAction::triggered, m_view, &WorkflowView::zoomIn);
    connect(aZoomOut, &QAction::triggered, m_view, &WorkflowView::zoomOut);
    connect(aZoomFit, &QAction::triggered, m_view, &WorkflowView::zoomToFit);
    connect(aZoomRst, &QAction::triggered, m_view, &WorkflowView::zoomReset);
}

void WorkflowEditorTab::setupConnections()
{
    connect(m_palette, &NodePalette::nodeTypeActivated,
            this, &WorkflowEditorTab::onPaletteDoubleClicked);

    // 选中节点 → 重建参数表单
    connect(m_scene, &QGraphicsScene::selectionChanged,
            this, &WorkflowEditorTab::onSelectionChanged);

    // 双击节点：若为 display.* 节点，弹出 QCustomPlot 可视化窗口
    connect(m_scene, &WorkflowScene::nodeDoubleClicked,
            this, &WorkflowEditorTab::onNodeDoubleClicked); // 双击弹图表功能

    // 引擎事件 → 状态栏 / 后台日志
    connect(m_engine, &IWorkflowEngine::nodeProgress, this,
            [this](const QString &id, int p, const QString &log)
            {
                qInfo().noquote() << QStringLiteral("[node %1] %2%  %3").arg(id).arg(p).arg(log);
            });
    connect(m_engine, &IWorkflowEngine::nodeRenamed, this,
            [this](const QString &id, const QString &)
            {
                if (id == m_scene->selectedNodeId())
                    rebuildParamForm(id);
            });
    connect(m_engine, &IWorkflowEngine::edgeRejected, this,
            [this](const EdgeInstance &, const QString &reason)
            {
                showStatus(tr("无法连线：%1").arg(reason), /*error*/ true);
            });
    connect(m_engine, &IWorkflowEngine::nodeStatusChanged, this,
            [this](const QString &id, NodeStatus s)
            {
                qInfo().noquote() << QStringLiteral("[node %1] -> %2").arg(id, toString(s));
                showStatus(tr("%1 %2").arg(id.left(8), toString(s)));
            });
    // 参数被外部改 → 如果当前正显示该节点，刷新表单
    connect(m_engine, &IWorkflowEngine::nodeParamsChanged, this,
            [this](const QString &id)
            {
                if (id == m_scene->selectedNodeId())
                    rebuildParamForm(id);
            });
    connect(m_engine, &IWorkflowEngine::runStarted, this, [this]
            { showStatus(tr("开始运行...")); });
    connect(m_engine, &IWorkflowEngine::runStopped, this, [this]
            { showStatus(tr("已停止")); });
    connect(m_engine, &IWorkflowEngine::runFinished, this,
            [this](bool ok)
            { showStatus(ok ? tr("全部完成") : tr("运行结束（有失败）"), !ok); });
}

QByteArray WorkflowEditorTab::currentWorkflowData() const
{
    return m_engine ? m_engine->serialize(false) : QByteArray();
}

bool WorkflowEditorTab::loadWorkflowData(const QByteArray &data)
{
    if (!m_engine)
        return false;
    return m_engine->deserialize(data);
}

// ----------------------------------------------- 工具栏槽

void WorkflowEditorTab::onActionNew()
{
    // 弹对话框确认清空
    if (QMessageBox::question(this, tr("新建"),
                              tr("确认清空当前工作流？")) == QMessageBox::Yes)
    {
        // 通知引擎清空所有节点和边
        m_engine->clear();
        m_currentFilePath.clear();
        m_currentIsTemplate = true;
        showStatus(tr("已新建空白工作流"));
    }
}

void WorkflowEditorTab::onActionOpenProject()
{
    emit requestOpenSavedWorkflow();
}

void WorkflowEditorTab::onActionSaveProject()
{
    emit requestSaveWorkflow();
}

void WorkflowEditorTab::onActionRunSingle()
{
    // 获取当前在画布上选中的节点
    auto id = m_scene->selectedNodeId();
    if (id.isEmpty())
    {
        showStatus(tr("请先在画布上选中一个节点"), true);
        return;
    }
    // 向引擎请求运行该节点
    m_engine->runSingle(id);
}

// 执行整个工作流（所有节点按拓扑排序自动运行）
void WorkflowEditorTab::onActionRunAll() { m_engine->runAll(); }
// 停止所有正在运行的节点
void WorkflowEditorTab::onActionStop() { m_engine->stop(); }

void WorkflowEditorTab::onPaletteDoubleClicked(const QString &typeId)
{
    // 从节点库拖拽到画布时，向引擎请求添加节点
    auto id = m_engine->addNode(typeId, QPointF(0, 0));
    if (id.isEmpty())
        showStatus(tr("无法创建节点：%1").arg(typeId), true);
    else
        showStatus(tr("已添加节点：%1").arg(typeId));
}

void WorkflowEditorTab::onSelectionChanged()
{
    // 当画布上的选中状态变化时，重建右侧属性面板表单
    rebuildParamForm(m_scene->selectedNodeId());
}

void WorkflowEditorTab::onNodeDoubleClicked(const QString &nodeId)
{
    if (nodeId.isEmpty() || !m_engine)
        return;
    NodeInstance info;
    bool found = false;
    for (const auto &n : m_engine->nodes())
    {
        if (n.instanceId == nodeId)
        {
            info = n;
            found = true;
            break;
        }
    }
    if (!found)
        return;

    // 取节点最近一次的输出文件（引擎里维护，可能为空）
    const QStringList outs = m_engine->outputsOf(nodeId);

    auto *dlg = new PlotDialog(info.displayName, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    // ① display.* 节点：画曲线（暂用占位演示数据；后续 TODO 改成
    //    解析 outs[0] 的 HDF5 → setSeries(...)）
    if (info.typeId.startsWith("display."))
    {
        const int curveCount = (info.typeId == "display.merge") ? 3 : 1;
        dlg->setDemoFromSeed(info.instanceId, curveCount);
        dlg->setMetaInfo(outs, tr("演示数据，等真实管线接入后替换为实际曲线。"));
    }
    // ② 其他节点：默认只显示输出文件信息，便于"跑完了看到底产出了啥"
    else
    {
        dlg->setMetaInfo(outs,
                         tr("节点类型: %1   状态: %2")
                             .arg(info.typeId, toString(m_engine->statusOf(nodeId))));
    }
    dlg->show();
}

// 把当前节点的 params 转成 QFormLayout（按 QVariant 类型选 widget）
void WorkflowEditorTab::rebuildParamForm(const QString &nodeId)
{
    if (!m_paramsForm || !m_paramsPanel)
        return;

    // 清空已有行
    while (m_paramsForm->count() > 0)
    {
        QLayoutItem *item = m_paramsForm->takeAt(0);
        if (!item)
            break;
        if (auto *w = item->widget())
            w->deleteLater();
        delete item;
    }

    if (nodeId.isEmpty())
    {
        m_paramsForm->addRow(new QLabel(tr("（未选中节点）"), m_paramsPanel));
        return;
    }

    // 从引擎查找当前节点
    NodeInstance info;
    bool found = false;
    for (const auto &n : m_engine->nodes())
        if (n.instanceId == nodeId)
        {
            info = n;
            found = true;
            break;
        }
    if (!found)
    {
        m_paramsForm->addRow(new QLabel(tr("（节点不存在）"), m_paramsPanel));
        return;
    }

    // 头部信息
    auto *lblHead = new QLabel(
        QStringLiteral("<b>%1</b><br><span style='color:#607D8B'>%2</span>")
            .arg(info.displayName, info.typeId),
        m_paramsPanel);
    lblHead->setTextFormat(Qt::RichText);
    m_paramsForm->addRow(lblHead);

    if (info.params.isEmpty())
    {
        m_paramsForm->addRow(new QLabel(tr("此节点无可配置参数"), m_paramsPanel));
        return;
    }

    // 按 key 字典序排，UI 稳定
    QStringList keys = info.params.keys();
    keys.sort();
    const QString id = nodeId; // capture by value

    for (const QString &key : keys)
    {
        const QVariant v = info.params.value(key);
        QWidget *editor = nullptr;

        switch (static_cast<QMetaType::Type>(v.typeId()))
        {
        case QMetaType::Bool:
        {
            auto *cb = new QCheckBox(m_paramsPanel);
            cb->setChecked(v.toBool());
            connect(cb, &QCheckBox::toggled, this, [this, id, key](bool b)
                    {
                    QVariantMap p;
                    for (const auto& n : m_engine->nodes())
                        if (n.instanceId == id) { p = n.params; break; }
                    p[key] = b;
                    m_engine->setNodeParams(id, p); });
            editor = cb;
            break;
        }
        case QMetaType::Int:
        case QMetaType::LongLong:
        case QMetaType::UInt:
        {
            auto *sp = new QSpinBox(m_paramsPanel);
            sp->setRange(-1000000000, 1000000000);
            sp->setValue(v.toInt());
            connect(sp, qOverload<int>(&QSpinBox::valueChanged), this, [this, id, key](int x)
                    {
                    QVariantMap p;
                    for (const auto& n : m_engine->nodes())
                        if (n.instanceId == id) { p = n.params; break; }
                    p[key] = x;
                    m_engine->setNodeParams(id, p); });
            editor = sp;
            break;
        }
        case QMetaType::Double:
        case QMetaType::Float:
        {
            auto *dsp = new QDoubleSpinBox(m_paramsPanel);
            dsp->setRange(-1e9, 1e9);
            dsp->setDecimals(4);
            dsp->setValue(v.toDouble());
            connect(dsp, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
                    [this, id, key](double x)
                    {
                        QVariantMap p;
                        for (const auto &n : m_engine->nodes())
                            if (n.instanceId == id)
                            {
                                p = n.params;
                                break;
                            }
                        p[key] = x;
                        m_engine->setNodeParams(id, p);
                    });
            editor = dsp;
            break;
        }
        default:
        {
            // 字符串：键名包含 path/file/dir 时附加文件浏览按钮
            const QString lk = key.toLower();
            const bool isPath = lk.contains("path") || lk.contains("file") || lk.contains("dir");
            if (isPath)
            {
                auto *row = new QWidget(m_paramsPanel);
                auto *h = new QHBoxLayout(row);
                h->setContentsMargins(0, 0, 0, 0);
                h->setSpacing(4);
                auto *le = new QLineEdit(v.toString(), row);
                auto *btn = new QToolButton(row);
                btn->setText(QStringLiteral("..."));
                btn->setToolTip(tr("浏览"));
                h->addWidget(le, 1);
                h->addWidget(btn);

                auto commit = [this, id, key, le]()
                {
                    QVariantMap p;
                    for (const auto &n : m_engine->nodes())
                        if (n.instanceId == id)
                        {
                            p = n.params;
                            break;
                        }
                    p[key] = le->text();
                    m_engine->setNodeParams(id, p);
                };
                connect(le, &QLineEdit::editingFinished, this, commit);
                const bool wantDir = lk.contains("dir");
                connect(btn, &QToolButton::clicked, this,
                        [this, le, commit, wantDir]()
                        {
                            QString picked;
                            if (wantDir)
                            {
                                picked = QFileDialog::getExistingDirectory(
                                    this, tr("选择目录"), le->text());
                            }
                            else
                            {
                                picked = QFileDialog::getOpenFileName(
                                    this, tr("选择文件"), le->text());
                            }
                            if (!picked.isEmpty())
                            {
                                le->setText(picked);
                                commit();
                            }
                        });
                editor = row;
            }
            else
            {
                auto *le = new QLineEdit(v.toString(), m_paramsPanel);
                connect(le, &QLineEdit::editingFinished, this, [this, id, key, le]
                        {
                        QVariantMap p;
                        for (const auto& n : m_engine->nodes())
                            if (n.instanceId == id) { p = n.params; break; }
                        p[key] = le->text();
                        m_engine->setNodeParams(id, p); });
                editor = le;
            }
            break;
        }
        }
        m_paramsForm->addRow(key, editor);
    }
}

void WorkflowEditorTab::showStatus(const QString &text, bool error, int timeoutMs)
{
    if (!m_statusLabel)
        return;
    m_statusLabel->setText(text);
    if (error)
        m_statusLabel->setStyleSheet("background:#FFEBEE; color:#C62828; font-weight:bold;");
    else
        m_statusLabel->setStyleSheet("background:#ECEFF1; color:#37474F;");
    if (timeoutMs > 0)
    {
        QTimer::singleShot(timeoutMs, this, [this]
                           {
            if (m_statusLabel) {
                m_statusLabel->setText(tr("就绪"));
                m_statusLabel->setStyleSheet("background:#ECEFF1; color:#37474F;");
            } });
    }
}
