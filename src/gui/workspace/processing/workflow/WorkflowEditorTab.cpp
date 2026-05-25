#include "WorkflowEditorTab.h"
#include "ParamRangePairUi.h"
#include "WorkflowScene.h"
#include "ParamRangePair.h"
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
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QDebug>
#include <QScrollArea>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLayoutItem>
#include <QToolButton>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QSet>

namespace {

/// 属性面板（右侧）最小宽度，避免拖窄后 SpinBox / 路径行被裁切
constexpr int kPropertyPanelMinWidth = 300;

QLabel *makeParamFormLabel(const QString &text, QWidget *parent)
{
    auto *lbl = new QLabel(text, parent);
    lbl->setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
    lbl->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    return lbl;
}

QString displayNativePath(const QString &path)
{
    return QDir::toNativeSeparators(path.trimmed());
}

QString variantFilesToDisplayText(const QVariant &val)
{
    QStringList lines;
    if (val.typeId() == QMetaType::QVariantList)
    {
        for (const QVariant &item : val.toList())
        {
            const QString native = displayNativePath(item.toString());
            if (!native.isEmpty())
                lines << native;
        }
    }
    return lines.join(QLatin1Char('\n'));
}

QString pathListToDisplayText(const QStringList &paths)
{
    QStringList lines;
    for (const QString &p : paths)
    {
        const QString native = displayNativePath(p);
        if (!native.isEmpty())
            lines << native;
    }
    return lines.join(QLatin1Char('\n'));
}

void applyIntSpinRange(QSpinBox *sp,
                       const QString &key,
                       const QMap<QString, double> &numMin,
                       const QMap<QString, bool> &numMinEx,
                       const QMap<QString, double> &numMax,
                       const QMap<QString, bool> &numMaxEx)
{
    int lo = -1000000000;
    int hi = 1000000000;
    if (numMin.contains(key))
    {
        const double lim = numMin.value(key);
        lo = numMinEx.value(key, false) ? static_cast<int>(lim) + 1 : static_cast<int>(lim);
    }
    if (numMax.contains(key))
    {
        const double lim = numMax.value(key);
        hi = numMaxEx.value(key, false) ? static_cast<int>(lim) - 1 : static_cast<int>(lim);
    }
    if (lo > hi)
        hi = lo;
    sp->setRange(lo, hi);
}

void applyDoubleSpinRange(QDoubleSpinBox *dsp,
                          const QString &key,
                          const QMap<QString, double> &numMin,
                          const QMap<QString, bool> &numMinEx,
                          const QMap<QString, double> &numMax,
                          const QMap<QString, bool> &numMaxEx)
{
    dsp->setRange(-1e9, 1e9);
    if (numMin.contains(key))
    {
        const double lim = numMin.value(key);
        if (numMinEx.value(key, false))
            dsp->setMinimum(lim + 1e-9);
        else
            dsp->setMinimum(lim);
    }
    if (numMax.contains(key))
    {
        const double lim = numMax.value(key);
        if (numMaxEx.value(key, false))
            dsp->setMaximum(lim - 1e-9);
        else
            dsp->setMaximum(lim);
    }
}

/// 甲方参数范围校验（编辑属性面板时即时提示）
bool validateNodeParams(const QString &typeId, const QVariantMap &params, QString *errorOut)
{
    auto fail = [errorOut](const QString &msg) -> bool
    {
        if (errorOut)
            *errorOut = msg;
        return false;
    };
    auto positive = [&](const char *key, const QString &msg) -> bool
    {
        if (params.value(QString::fromUtf8(key)).toDouble() <= 0.0)
            return fail(msg);
        return true;
    };
    if (typeId == QLatin1String("preprocess.depth_correct"))
    {
        if (params.value(QStringLiteral("depth_step")).toDouble() <= 0.0)
            return fail(QObject::tr("深度步长必须大于 0"));
    }
    else if (typeId == QLatin1String("preprocess.depth_time_correct"))
    {
        if (params.value(QStringLiteral("time_scale")).toDouble() <= 0.0)
            return fail(QObject::tr("时间缩放系数必须大于 0"));
        if (params.contains(QStringLiteral("depth_scale"))
            && params.value(QStringLiteral("depth_scale")).toDouble() <= 0.0)
            return fail(QObject::tr("深度缩放系数必须大于 0"));
    }
    else if (typeId == QLatin1String("preprocess.data_merge"))
    {
        if (params.value(QStringLiteral("sample_points")).toInt() < 0)
            return fail(QObject::tr("采样点数不能小于 0"));
    }
    else if (typeId == QLatin1String("preprocess.das_convert"))
    {
        if (!positive("fs", QObject::tr("采样率必须大于 0")))
            return false;
        if (params.value(QStringLiteral("win_len")).toInt() < 1)
            return fail(QObject::tr("窗口长度必须大于等于 1"));
        const QString clip = params.value(QStringLiteral("clip_value")).toString().trimmed();
        if (!clip.isEmpty())
        {
            bool ok = false;
            const double v = clip.toDouble(&ok);
            if (!ok || v < 0.0)
                return fail(QObject::tr("幅值裁剪须为空或大于等于 0 的数值"));
        }
    }
    else if (typeId == QLatin1String("preprocess.lfdas_extract"))
    {
        if (!positive("fs", QObject::tr("采样率必须大于 0")))
            return false;
        if (params.value(QStringLiteral("win_len")).toInt() < 1
            || params.value(QStringLiteral("order")).toInt() < 1
            || params.value(QStringLiteral("down_factor")).toInt() < 1)
            return fail(QObject::tr("窗口长度、滤波器阶数、降采样系数均须 ≥ 1"));
    }
    else if (typeId == QLatin1String("preprocess.fk_analysis"))
    {
        if (!positive("fs", QObject::tr("采样率必须大于 0")))
            return false;
        if (!positive("dx", QObject::tr("深度/通道间距必须大于 0")))
            return false;
    }
    else if (typeId == QLatin1String("preprocess.fft_extract"))
    {
        const double fs = params.value(QStringLiteral("fs")).toDouble();
        if (fs <= 0.0)
            return fail(QObject::tr("采样率必须大于 0"));
        const double overlap = params.value(QStringLiteral("overlap")).toDouble();
        if (overlap < 0.0 || overlap >= 1.0)
            return fail(QObject::tr("窗口重叠率须满足 0 ≤ overlap < 1"));
        if (params.value(QStringLiteral("win_len")).toInt() < 1)
            return fail(QObject::tr("FFT 窗口长度须 ≥ 1"));
    }
    else if (typeId == QLatin1String("preprocess.bandpass_filter"))
    {
        if (!positive("fs", QObject::tr("采样率必须大于 0")))
            return false;
        if (params.value(QStringLiteral("order")).toInt() < 1)
            return fail(QObject::tr("滤波器阶数须 ≥ 1"));
    }
    else if (typeId == QLatin1String("preprocess.downsample"))
    {
        const double fs = params.value(QStringLiteral("fs")).toDouble();
        if (fs <= 0.0)
            return fail(QObject::tr("采样率必须大于 0"));
        if (params.value(QStringLiteral("factor")).toInt() < 2)
            return fail(QObject::tr("降采样系数须 ≥ 2"));
        const QString newFsText = params.value(QStringLiteral("new_fs")).toString().trimmed();
        if (!newFsText.isEmpty())
        {
            bool ok = false;
            const double newFs = newFsText.toDouble(&ok);
            if (!ok || newFs <= 0.0 || newFs >= fs)
                return fail(QObject::tr("目标采样率须为空或满足 0 < new_fs < fs"));
        }
    }
    else if (typeId == QLatin1String("preprocess.fts_extract"))
    {
        const double fs = params.value(QStringLiteral("fs")).toDouble();
        if (fs <= 0.0)
            return fail(QObject::tr("采样率必须大于 0"));
        const double overlap = params.value(QStringLiteral("overlap")).toDouble();
        if (overlap < 0.0 || overlap >= 1.0)
            return fail(QObject::tr("重叠率须满足 0 ≤ overlap < 1"));
        if (params.value(QStringLiteral("win_len")).toInt() < 1)
            return fail(QObject::tr("STFT 窗口长度须 ≥ 1"));
    }
    else if (typeId == QLatin1String("preprocess.mainfreq_split"))
    {
        if (!positive("fs", QObject::tr("采样率必须大于 0")))
            return false;
    }
    else if (typeId == QLatin1String("preprocess.fbe_extract"))
    {
        if (!positive("fs", QObject::tr("采样率必须大于 0")))
            return false;
        if (params.value(QStringLiteral("win_len")).toInt() < 1)
            return fail(QObject::tr("窗口长度须 ≥ 1"));
        if (params.value(QStringLiteral("smooth_win")).toInt() < 0)
            return fail(QObject::tr("平滑窗口须 ≥ 0"));
    }
    else if (typeId == QLatin1String("preprocess.moving_avg"))
    {
        if (params.value(QStringLiteral("win_len")).toInt() < 1)
            return fail(QObject::tr("窗口长度须 ≥ 1"));
    }
    else if (typeId == QLatin1String("preprocess.slow_strain"))
    {
        if (!positive("fs", QObject::tr("采样率必须大于 0")))
            return false;
        if (!positive("cut_freq", QObject::tr("截止频率必须大于 0")))
            return false;
        if (params.value(QStringLiteral("poly_order")).toInt() < 1)
            return fail(QObject::tr("多项式阶数须 ≥ 1"));
    }
    else if (typeId == QLatin1String("preprocess.dts_denoise"))
    {
        if (params.value(QStringLiteral("win_len")).toInt() < 1)
            return fail(QObject::tr("窗口长度须 ≥ 1"));
    }
    else if (typeId == QLatin1String("preprocess.temp_correct"))
    {
        if (params.value(QStringLiteral("poly_order")).toInt() < 1)
            return fail(QObject::tr("多项式阶数须 ≥ 1"));
    }
    else if (typeId == QLatin1String("preprocess.temp_diff"))
    {
        if (params.value(QStringLiteral("base_time")).toDouble() < 0.0)
            return fail(QObject::tr("本底时间须 ≥ 0"));
        const int ord = params.value(QStringLiteral("order")).toInt();
        if (ord != 1 && ord != 2)
            return fail(QObject::tr("差分阶数须为 1 或 2"));
    }
    else if (typeId == QLatin1String("preprocess.dtgs_gradient"))
    {
        if (params.value(QStringLiteral("depth_step")).toDouble() <= 0.0)
            return fail(QObject::tr("深度步长必须大于 0"));
        const int bo = params.value(QStringLiteral("boundary_order")).toInt();
        if (bo < 1 || bo > 3)
            return fail(QObject::tr("边界阶数须为 1、2 或 3"));
    }
    else if (typeId == QLatin1String("preprocess.baseline_build"))
    {
        if (params.value(QStringLiteral("poly_order")).toInt() < 1)
            return fail(QObject::tr("多项式阶数须 ≥ 1"));
    }
    else if (typeId == QLatin1String("preprocess.strain_disturb"))
    {
        if (!positive("fs", QObject::tr("采样率必须大于 0")))
            return false;
        if (params.value(QStringLiteral("enhance")).toDouble() < 1.0)
            return fail(QObject::tr("增强系数须 ≥ 1.0"));
        if (!positive("hp_freq", QObject::tr("高通截止频率必须大于 0")))
            return false;
    }
    else if (typeId == QLatin1String("preprocess.temp_coupling"))
    {
        if (params.value(QStringLiteral("poly_order")).toInt() < 1)
            return fail(QObject::tr("多项式阶数须 ≥ 1"));
    }

    const QList<processing::ParamRangePairSpec> rangeSpecs =
        processing::paramRangePairsForTypeId(typeId);
    if (!rangeSpecs.isEmpty()
        && !processing::validateParamRangePairs(params, rangeSpecs, errorOut))
        return false;

    return true;
}

} // namespace

using namespace processing;
using namespace processing::gui;

WorkflowEditorTab::WorkflowEditorTab(IWorkflowEngine *engine,
                                     INodeFactory *factory,
                                     QWidget *parent)
    : QWidget(parent), m_engine(engine), m_factory(factory)
{
    setupUi();
    setupConnections();
    updateSavedWorkflowSnapshot();
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
    m_toolbar->addAction(tr("清空画布"), this, &WorkflowEditorTab::onActionClearCanvas);
    m_toolbar->addAction(tr("打开预设"), this, &WorkflowEditorTab::onActionOpenProject);
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
    m_toolbar->addSeparator();
    auto *aTestSeq = m_toolbar->addAction(tr("显示序号角标"));
    aTestSeq->setCheckable(true);
    aTestSeq->setChecked(false);
    aTestSeq->setToolTip(tr("显示/隐藏节点标题旁的甲方序号角标（悬停节点仍可看 tooltip 序号）"));
    connect(aTestSeq, &QAction::toggled, this, [this, aTestSeq](bool on)
            {
        if (m_scene)
            m_scene->setTestSeqBadgeVisible(on);
        aTestSeq->setText(on ? tr("隐藏序号角标") : tr("显示序号角标"));
    });
    if (auto *btnSeq = qobject_cast<QToolButton *>(m_toolbar->widgetForAction(aTestSeq)))
    {
        btnSeq->setStyleSheet("QToolButton { color: #37474F; background: transparent; }"
                              "QToolButton:checked { background: rgba(230, 81, 0, 0.12); }");
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
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setMinimumWidth(kPropertyPanelMinWidth);
    m_paramsPanel = new QWidget(scroll);
    m_paramsPanel->setMinimumWidth(kPropertyPanelMinWidth);
    m_paramsPanel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    auto *panelVBox = new QVBoxLayout(m_paramsPanel);
    panelVBox->setContentsMargins(4, 4, 4, 4);
    panelVBox->setSpacing(8);
    m_paramsForm = new QFormLayout();
    m_paramsForm->setContentsMargins(0, 0, 0, 0);
    m_paramsForm->setSpacing(10);
    m_paramsForm->setLabelAlignment(Qt::AlignJustify | Qt::AlignVCenter);
    m_paramsForm->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_paramsForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    m_paramsForm->setRowWrapPolicy(QFormLayout::DontWrapRows);
    panelVBox->addLayout(m_paramsForm);
    panelVBox->addStretch(1);
    m_paramsForm->addRow(new QLabel(tr("（未选中节点）"), m_paramsPanel));
    scroll->setWidget(m_paramsPanel);
    rl->addWidget(scroll, 1);

    m_splitter->addWidget(m_palette);
    m_splitter->addWidget(m_view);
    m_splitter->addWidget(right);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setStretchFactor(2, 0);
    right->setMinimumWidth(kPropertyPanelMinWidth);
    m_splitter->setCollapsible(2, false);
    m_splitter->setSizes({220, 800, 320});

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
            this, &WorkflowEditorTab::onNodeDoubleClicked);

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
    connect(m_engine, &IWorkflowEngine::edgeAdded, this,
            [this](const EdgeInstance &edge)
            {
                const QString sel = m_scene->selectedNodeId();
                if (sel == edge.toNode || sel == edge.fromNode)
                    rebuildParamForm(sel);
            });
    connect(m_engine, &IWorkflowEngine::edgeRemoved, this,
            [this](const EdgeInstance &edge)
            {
                const QString sel = m_scene->selectedNodeId();
                if (sel == edge.toNode || sel == edge.fromNode)
                    rebuildParamForm(sel);
            });
    connect(m_engine, &IWorkflowEngine::runStarted, this, [this]
            { showStatus(tr("开始运行...")); });
    connect(m_engine, &IWorkflowEngine::runStopped, this, [this]
            { showStatus(tr("已停止")); });
    connect(m_engine, &IWorkflowEngine::runFinished, this,
            [this](bool ok)
            { showStatus(ok ? tr("全部完成") : tr("运行结束（有失败）"), !ok); });
    connect(m_engine, &IWorkflowEngine::nodeFinished, this,
            [this](const QString &, const NodeRunResult &)
            {
                const QString sel = m_scene->selectedNodeId();
                if (sel.isEmpty() || !m_engine)
                    return;
                for (const auto &n : m_engine->nodes())
                {
                    if (n.instanceId == sel
                        && n.typeId == QLatin1String("preprocess.format_convert"))
                    {
                        rebuildParamForm(sel);
                        break;
                    }
                }
            });
}

QByteArray WorkflowEditorTab::currentWorkflowData() const
{
    return m_engine ? m_engine->serialize(false) : QByteArray();
}

bool WorkflowEditorTab::hasUnsavedChanges() const
{
    return currentWorkflowData() != m_savedWorkflowData;
}

void WorkflowEditorTab::updateSavedWorkflowSnapshot()
{
    m_savedWorkflowData = currentWorkflowData();
}

bool WorkflowEditorTab::loadWorkflowData(const QByteArray &data)
{
    if (!m_engine)
        return false;
    const bool success = m_engine->deserialize(data);
    if (success)
        updateSavedWorkflowSnapshot();
    return success;
}

// ----------------------------------------------- 工具栏槽

void WorkflowEditorTab::onActionClearCanvas()
{
    emit requestClearCanvas();
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
    Q_UNUSED(nodeId);
    // 甲方确认前：不在双击时打开样例配置窗/图表窗，统一在右侧属性面板编辑
    showStatus(tr("请在右侧属性面板编辑节点参数。"));
}

// 把当前节点的 params 转成 QFormLayout（按 QVariant 类型选 widget）
void WorkflowEditorTab::rebuildParamForm(const QString &nodeId)
{
    if (m_rebuildingParamForm)
        return;
    if (!m_paramsForm || !m_paramsPanel)
        return;

    m_rebuildingParamForm = true;
    struct RebuildGuard
    {
        bool &flag;
        ~RebuildGuard() { flag = false; }
    } guard{m_rebuildingParamForm};

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
    const QString id = nodeId; // capture by value

    // 获取节点元数据：中文标签 + 参数顺序，并与实例参数合并
    QMap<QString, QString> paramLabels;
    QStringList paramOrder;
    bool paramsFictional = false;
    bool hidePropertyPanel = false;
    QString configDialogId;
    QMap<QString, QStringList> paramOptions;
    QMap<QString, double> paramFloatMin;
    QMap<QString, bool> paramFloatMinExclusive;
    QMap<QString, double> paramFloatMax;
    QMap<QString, bool> paramFloatMaxExclusive;
    QMap<QString, bool> paramRequired;
    QVariantMap displayParams = info.params;
    if (m_factory)
    {
        for (const auto &meta : m_factory->listAll())
        {
            if (meta.typeId == info.typeId)
            {
                paramLabels = meta.paramLabels;
                paramOrder = meta.paramOrder;
                paramOptions = meta.paramOptions;
                paramFloatMin = meta.paramFloatMin;
                paramFloatMinExclusive = meta.paramFloatMinExclusive;
                paramFloatMax = meta.paramFloatMax;
                paramFloatMaxExclusive = meta.paramFloatMaxExclusive;
                paramRequired = meta.paramRequired;
                paramsFictional = meta.clientParamsFictional;
                hidePropertyPanel = meta.hidePropertyPanel;
                configDialogId = meta.configDialogId;
                displayParams = meta.defaultParams;
                for (auto it = info.params.constBegin(); it != info.params.constEnd(); ++it)
                    displayParams.insert(it.key(), it.value());
                // 旧工作流/预设里 params 为空或不完整时，自动补齐并写回引擎
                if (!hidePropertyPanel && displayParams.size() > info.params.size())
                    m_engine->setNodeParams(id, displayParams);
                break;
            }
        }
    }

    const QString fictionalNote =
        paramsFictional
            ? tr("<br><span style='color:#E65100'>（参数为行业模板，暂为虚构，待甲方确认）</span>")
            : QString();
    auto *lblHead = new QLabel(
        QStringLiteral("<b>%1</b><br><span style='color:#607D8B'>%2</span>%3")
            .arg(info.displayName, info.typeId, fictionalNote),
        m_paramsPanel);
    lblHead->setTextFormat(Qt::RichText);
    lblHead->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    m_paramsForm->addRow(QString(), lblHead);

    if (hidePropertyPanel)
    {
        auto *lblHint = new QLabel(tr("此节点暂不在右侧展示参数。"), m_paramsPanel);
        lblHint->setWordWrap(true);
        lblHint->setStyleSheet(QStringLiteral("color:#546E7A; padding: 8px 0;"));
        m_paramsForm->addRow(QString(), lblHint);
        return;
    }

    if (info.typeId == QLatin1String("input.data_input"))
    {
        rebuildDataInputParamForm(id, displayParams);
        return;
    }

    if (info.typeId == QLatin1String("preprocess.format_convert"))
    {
        rebuildFormatConvertParamForm(id, displayParams);
        return;
    }

    if (displayParams.isEmpty())
    {
        m_paramsForm->addRow(new QLabel(tr("此节点无可配置参数"), m_paramsPanel));
        return;
    }

    const QList<processing::ParamRangePairSpec> rangeSpecs =
        processing::paramRangePairsForTypeId(info.typeId);
    if (!rangeSpecs.isEmpty())
    {
        const bool migrated = processing::migrateParamRangePairs(displayParams, rangeSpecs);
        const QVariantMap beforeClamp = displayParams;
        processing::clampParamRangePairs(displayParams, rangeSpecs);
        if (migrated || beforeClamp != displayParams)
            m_engine->setNodeParams(id, displayParams);
    }

    QStringList keys;
    if (!paramOrder.isEmpty())
    {
        keys = paramOrder;
        // 仅展示 node_client_params.json 声明的字段，丢弃旧版 Excel 占位字段
        QVariantMap pruned;
        for (const QString &k : keys)
        {
            if (displayParams.contains(k))
                pruned.insert(k, displayParams.value(k));
        }
        if (pruned.size() != info.params.size())
            m_engine->setNodeParams(id, pruned);
        displayParams = pruned;
    }
    else
    {
        keys = displayParams.keys();
        keys.sort();
    }

    {
        QStringList uniqueKeys;
        for (const QString &k : keys)
        {
            if (!displayParams.contains(k) || uniqueKeys.contains(k))
                continue;
            uniqueKeys.append(k);
        }
        keys = uniqueKeys;
    }

    const QString typeId = info.typeId;
    auto applyParams = [this, id, typeId](QVariantMap p)
    {
        QString err;
        if (!validateNodeParams(typeId, p, &err))
        {
            showStatus(err, true);
            return;
        }
        m_engine->setNodeParams(id, p);
    };

    QSet<QString> rangeSkipKeys;
    for (const processing::ParamRangePairSpec &rs : rangeSpecs)
    {
        rangeSkipKeys.insert(rs.maxKey);
        rangeSkipKeys.insert(rs.legacyKey);
    }

    auto fetchNodeParams = [this, id]()
    {
        QVariantMap p;
        for (const auto &n : m_engine->nodes())
            if (n.instanceId == id)
            {
                p = n.params;
                break;
            }
        return p;
    };

    for (const QString &key : keys)
    {
        if (rangeSkipKeys.contains(key))
            continue;

        bool rangeRowAdded = false;
        for (const processing::ParamRangePairSpec &rs : rangeSpecs)
        {
            if (key != rs.minKey)
                continue;
            auto applyMut = [applyParams](QVariantMap &p) { applyParams(p); };
            processing::gui::addParamRangePairRow(m_paramsForm, m_paramsPanel, rs, fetchNodeParams,
                                                  applyMut);
            rangeRowAdded = true;
            break;
        }
        if (rangeRowAdded)
            continue;

        const QVariant v = displayParams.value(key);
        QWidget *editor = nullptr;
        const bool hasEnumOptions =
            paramOptions.contains(key) && !paramOptions.value(key).isEmpty();

        switch (static_cast<QMetaType::Type>(v.typeId()))
        {
        case QMetaType::Bool:
        {
            auto *cb = new QCheckBox(m_paramsPanel);
            cb->setChecked(v.toBool());
            connect(cb, &QCheckBox::toggled, this, [this, id, key, applyParams](bool b)
                    {
                    QVariantMap p;
                    for (const auto& n : m_engine->nodes())
                        if (n.instanceId == id) { p = n.params; break; }
                    p[key] = b;
                    applyParams(p); });
            editor = cb;
            break;
        }
        case QMetaType::Int:
        case QMetaType::LongLong:
        case QMetaType::UInt:
        {
            auto *sp = new QSpinBox(m_paramsPanel);
            applyIntSpinRange(sp, key, paramFloatMin, paramFloatMinExclusive, paramFloatMax,
                              paramFloatMaxExclusive);
            const int val = qBound(sp->minimum(), v.toInt(), sp->maximum());
            sp->setValue(val);
            if (val != v.toInt())
            {
                QVariantMap p;
                for (const auto &n : m_engine->nodes())
                    if (n.instanceId == id)
                    {
                        p = n.params;
                        break;
                    }
                p[key] = val;
                applyParams(p);
            }
            connect(sp, qOverload<int>(&QSpinBox::valueChanged), this, [this, id, key, applyParams](int x)
                    {
                    QVariantMap p;
                    for (const auto& n : m_engine->nodes())
                        if (n.instanceId == id) { p = n.params; break; }
                    p[key] = x;
                    applyParams(p); });
            editor = sp;
            break;
        }
        case QMetaType::QVariantList:
        {
            auto formatList = [](const QVariant &val) -> QString
            {
                QStringList parts;
                for (const QVariant &item : val.toList())
                    parts << item.toString();
                return parts.join(QLatin1Char(','));
            };
            auto *le = new QLineEdit(formatList(v), m_paramsPanel);
            le->setPlaceholderText(tr("逗号分隔，如 0,100"));
            connect(le, &QLineEdit::editingFinished, this, [this, id, key, le, applyParams]
                    {
                        QVariantMap p;
                        for (const auto &n : m_engine->nodes())
                            if (n.instanceId == id)
                            {
                                p = n.params;
                                break;
                            }
                        QVariantList list;
                        const QStringList parts =
                            le->text().split(QLatin1Char(','), Qt::SkipEmptyParts);
                        for (QString part : parts)
                        {
                            part = part.trimmed();
                            bool ok = false;
                            const double d = part.toDouble(&ok);
                            if (ok)
                                list.append(d);
                            else
                                list.append(part);
                        }
                        p[key] = list;
                        applyParams(p);
                    });
            editor = le;
            break;
        }
        case QMetaType::Double:
        case QMetaType::Float:
        {
            auto *dsp = new QDoubleSpinBox(m_paramsPanel);
            dsp->setDecimals(6);
            applyDoubleSpinRange(dsp, key, paramFloatMin, paramFloatMinExclusive, paramFloatMax,
                                 paramFloatMaxExclusive);
            const double val =
                qBound(dsp->minimum(), v.toDouble(), dsp->maximum());
            dsp->setValue(val);
            if (qAbs(val - v.toDouble()) > 1e-6)
            {
                QVariantMap p;
                for (const auto &n : m_engine->nodes())
                    if (n.instanceId == id)
                    {
                        p = n.params;
                        break;
                    }
                p[key] = val;
                applyParams(p);
            }
            connect(dsp, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
                    [this, id, key, applyParams](double x)
                    {
                        QVariantMap p;
                        for (const auto &n : m_engine->nodes())
                            if (n.instanceId == id)
                            {
                                p = n.params;
                                break;
                            }
                        p[key] = x;
                        applyParams(p);
                    });
            editor = dsp;
            break;
        }
        default:
        {
            if (hasEnumOptions)
            {
                auto *combo = new QComboBox(m_paramsPanel);
                combo->addItems(paramOptions.value(key));
                const int idx = combo->findText(v.toString());
                combo->setCurrentIndex(idx >= 0 ? idx : 0);
                connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), this,
                        [this, id, key, combo, applyParams](int)
                        {
                            QVariantMap p;
                            for (const auto &n : m_engine->nodes())
                                if (n.instanceId == id)
                                {
                                    p = n.params;
                                    break;
                                }
                            p[key] = combo->currentText();
                            applyParams(p);
                        });
                editor = combo;
                break;
            }

            // 字符串：键名包含 path/file/dir 时附加文件浏览按钮
            const QString lk = key.toLower();
            const bool isPath = lk.contains("path") || lk.contains("file") || lk.contains("dir");
            if (isPath)
            {
                auto *row = new QWidget(m_paramsPanel);
                auto *h = new QHBoxLayout(row);
                h->setContentsMargins(0, 0, 0, 0);
                h->setSpacing(4);
                auto *le = new QLineEdit(displayNativePath(v.toString()), row);
                auto *btn = new QToolButton(row);
                btn->setText(QStringLiteral("..."));
                btn->setToolTip(tr("浏览"));
                h->addWidget(le, 1);
                h->addWidget(btn);

                auto commit = [this, id, key, le, applyParams]()
                {
                    QVariantMap p;
                    for (const auto &n : m_engine->nodes())
                        if (n.instanceId == id)
                        {
                            p = n.params;
                            break;
                        }
                    p[key] = displayNativePath(le->text());
                    applyParams(p);
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
                                le->setText(displayNativePath(picked));
                                commit();
                            }
                        });
                editor = row;
            }
            else
            {
                auto *le = new QLineEdit(v.toString(), m_paramsPanel);
                connect(le, &QLineEdit::editingFinished, this, [this, id, key, le, applyParams]
                        {
                        QVariantMap p;
                        for (const auto& n : m_engine->nodes())
                            if (n.instanceId == id) { p = n.params; break; }
                        p[key] = le->text();
                        applyParams(p); });
                editor = le;
            }
            break;
        }
        }
        QString labelText = paramLabels.value(key, key);
        if (paramRequired.value(key, false))
            labelText += QStringLiteral(" *");
        m_paramsForm->addRow(makeParamFormLabel(labelText, m_paramsPanel), editor);
    }
}

void WorkflowEditorTab::rebuildDataInputParamForm(const QString &nodeId,
                                                  const QVariantMap &displayParams)
{
    auto *inputField = new QWidget(m_paramsPanel);
    auto *inputVBox = new QVBoxLayout(inputField);
    inputVBox->setContentsMargins(0, 0, 0, 0);
    inputVBox->setSpacing(6);
    auto *btnPick = new QPushButton(tr("选择输入文件…"), inputField);
    btnPick->setFixedHeight(30);
    btnPick->setToolTip(tr("当前仅支持 LAS（*.las）；后续可在参数表中扩展格式"));
    auto *fileList = new QTextEdit(inputField);
    fileList->setReadOnly(true);
    fileList->setFixedHeight(96);
    fileList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    fileList->setPlaceholderText(tr("未选择文件"));
    fileList->setPlainText(variantFilesToDisplayText(displayParams.value(QStringLiteral("input_files"))));
    inputVBox->addWidget(btnPick);
    inputVBox->addWidget(fileList);

    m_paramsForm->addRow(makeParamFormLabel(tr("输入文件"), m_paramsPanel), inputField);

    const QString fmt = displayParams.value(QStringLiteral("file_format"), QStringLiteral("LAS")).toString();
    auto *fmtLabel = new QLabel(fmt, m_paramsPanel);
    fmtLabel->setStyleSheet(QStringLiteral("color:#546E7A;"));
    m_paramsForm->addRow(makeParamFormLabel(tr("文件格式"), m_paramsPanel), fmtLabel);

    connect(btnPick, &QPushButton::clicked, this,
            [this, nodeId, fileList]()
            {
                const QStringList paths = QFileDialog::getOpenFileNames(
                    this,
                    tr("选择 LAS 文件"),
                    QString(),
                    tr("LAS 文件 (*.las);;所有文件 (*)"));
                if (paths.isEmpty())
                    return;

                QVariantList files;
                for (const QString &p : paths)
                    files.append(displayNativePath(p));

                QVariantMap p;
                for (const auto &n : m_engine->nodes())
                {
                    if (n.instanceId == nodeId)
                    {
                        p = n.params;
                        break;
                    }
                }
                p.insert(QStringLiteral("input_files"), files);
                p.insert(QStringLiteral("file_format"), QStringLiteral("LAS"));
                m_engine->setNodeParams(nodeId, p);
                fileList->setPlainText(variantFilesToDisplayText(files));
                showStatus(tr("已选择 %1 个文件").arg(paths.size()));
            });
}

void WorkflowEditorTab::rebuildFormatConvertParamForm(const QString &nodeId,
                                                      const QVariantMap &displayParams)
{
    const QStringList upstreamFiles =
        m_engine ? m_engine->upstreamInputFiles(nodeId, QStringLiteral("in")) : QStringList{};

    auto *hint = new QLabel(tr("输入 LAS 由上游「数据输入」经连线传入；运行本节点或「全部运行」时自动使用下列文件。"),
                            m_paramsPanel);
    hint->setWordWrap(true);
    hint->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    hint->setStyleSheet(QStringLiteral("color:#546E7A; padding: 0;"));
    m_paramsForm->addRow(QString(), hint);

    auto *inFileList = new QTextEdit(m_paramsPanel);
    inFileList->setReadOnly(true);
    inFileList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    inFileList->setFixedHeight(96);
    if (upstreamFiles.isEmpty())
    {
        inFileList->setPlaceholderText(tr("未连接上游或未选择 LAS。请将「数据输入」的「数据文件」口连到本节点「输入」口。"));
    }
    else
    {
        inFileList->setPlainText(pathListToDisplayText(upstreamFiles));
    }
    m_paramsForm->addRow(makeParamFormLabel(tr("上游输入文件"), m_paramsPanel), inFileList);

    auto *comboOut = new QComboBox(m_paramsPanel);
    comboOut->addItem(QStringLiteral("H5"));
    comboOut->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    comboOut->setFixedHeight(28);
    const QString outType = displayParams.value(QStringLiteral("output_type"), QStringLiteral("H5")).toString();
    const int idx = comboOut->findText(outType);
    comboOut->setCurrentIndex(idx >= 0 ? idx : 0);

    auto *dirRow = new QWidget(m_paramsPanel);
    auto *dirHBox = new QHBoxLayout(dirRow);
    dirHBox->setContentsMargins(0, 0, 0, 0);
    dirHBox->setSpacing(6);
    auto *leDir = new QLineEdit(
        displayNativePath(displayParams.value(QStringLiteral("output_dir")).toString()), dirRow);
    leDir->setPlaceholderText(tr("请选择输出目录"));
    leDir->setMinimumHeight(28);
    leDir->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto *btnDir = new QToolButton(dirRow);
    btnDir->setText(QStringLiteral("..."));
    btnDir->setToolTip(tr("选择输出目录"));
    btnDir->setFixedSize(32, 28);
    dirHBox->addWidget(leDir, 1);
    dirHBox->addWidget(btnDir, 0, Qt::AlignVCenter);

    auto commit = [this, nodeId, comboOut, leDir]()
    {
        QVariantMap p;
        for (const auto &n : m_engine->nodes())
        {
            if (n.instanceId == nodeId)
            {
                p = n.params;
                break;
            }
        }
        p.insert(QStringLiteral("output_type"), comboOut->currentText());
        p.insert(QStringLiteral("output_dir"), displayNativePath(leDir->text()));
        m_engine->setNodeParams(nodeId, p);
    };

    connect(comboOut, qOverload<int>(&QComboBox::currentIndexChanged), this, commit);
    connect(leDir, &QLineEdit::editingFinished, this, commit);
    connect(btnDir, &QToolButton::clicked, this,
            [this, leDir, commit]()
            {
                const QString picked = QFileDialog::getExistingDirectory(
                    this, tr("选择输出目录"), leDir->text());
                if (!picked.isEmpty())
                {
                    leDir->setText(displayNativePath(picked));
                    commit();
                }
            });

    auto *btnExport = new QPushButton(tr("导出"), m_paramsPanel);
    btnExport->setToolTip(tr("将节点已生成的 H5 文件另存到指定位置"));
    btnExport->setFixedHeight(30);
    btnExport->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnExport, &QPushButton::clicked, this, [this, nodeId]()
            { onExportFormatConvert(nodeId); });

    m_paramsForm->addRow(makeParamFormLabel(tr("输出格式"), m_paramsPanel), comboOut);
    m_paramsForm->addRow(makeParamFormLabel(tr("输出目录"), m_paramsPanel), dirRow);

    auto *exportWrap = new QWidget(m_paramsPanel);
    auto *exportHBox = new QHBoxLayout(exportWrap);
    exportHBox->setContentsMargins(0, 4, 0, 0);
    exportHBox->addWidget(btnExport);
    m_paramsForm->addRow(QString(), exportWrap);
}

void WorkflowEditorTab::onExportFormatConvert(const QString &nodeId)
{
    if (!m_engine)
        return;

    auto collectH5InDir = [](const QString &dirPath) -> QStringList
    {
        QStringList found;
        const QDir dir(dirPath);
        const QStringList names = dir.entryList(
            {QStringLiteral("*.h5"), QStringLiteral("*.hdf5")},
            QDir::Files);
        for (const QString &name : names)
            found.append(dir.absoluteFilePath(name));
        return found;
    };

    QStringList h5Files = m_engine->outputsOf(nodeId);
    QStringList filtered;
    for (const QString &p : h5Files)
    {
        const QString low = p.toLower();
        if (low.endsWith(QStringLiteral(".h5")) || low.endsWith(QStringLiteral(".hdf5")))
            filtered.append(p);
    }
    h5Files = filtered;

    if (h5Files.isEmpty())
    {
        for (const auto &n : m_engine->nodes())
        {
            if (n.instanceId != nodeId)
                continue;
            const QString outDir = n.params.value(QStringLiteral("output_dir")).toString().trimmed();
            if (!outDir.isEmpty())
                h5Files = collectH5InDir(outDir);
            break;
        }
    }

    if (h5Files.isEmpty())
    {
        showStatus(tr("没有可导出的 H5 文件，请先运行节点完成转换"), true);
        return;
    }

    int copied = 0;
    if (h5Files.size() == 1)
    {
        const QFileInfo srcFi(h5Files.constFirst());
        const QString destPath = QFileDialog::getSaveFileName(
            this,
            tr("导出 H5 文件"),
            srcFi.absoluteFilePath(),
            tr("HDF5 文件 (*.h5 *.hdf5);;所有文件 (*)"));
        if (destPath.isEmpty())
            return;

        if (QFile::exists(destPath))
            QFile::remove(destPath);
        if (QFile::copy(h5Files.constFirst(), destPath))
        {
            copied = 1;
            showStatus(tr("已导出：%1").arg(destPath));
        }
        else
        {
            showStatus(tr("导出失败：无法写入 %1").arg(destPath), true);
        }
        return;
    }

    const QString destDir = QFileDialog::getExistingDirectory(
        this, tr("导出到文件夹"), QString());
    if (destDir.isEmpty())
        return;

    for (const QString &src : h5Files)
    {
        const QFileInfo fi(src);
        if (!fi.exists())
            continue;
        const QString dest = QDir(destDir).filePath(fi.fileName());
        if (QFile::exists(dest))
            QFile::remove(dest);
        if (QFile::copy(src, dest))
            ++copied;
    }

    if (copied > 0)
        showStatus(tr("已导出 %1/%2 个 H5 文件到 %3").arg(copied).arg(h5Files.size()).arg(destDir));
    else
        showStatus(tr("导出失败，请检查目标目录权限"), true);
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
