#include "NodeCompatibility.h"

#include <QObject>

namespace processing
{

namespace
{

/// 构造预处理节点画像的简便写法
NodeCompatProfile makePreprocess(SensorFamily family, DataTier tier, bool terminal = false,
                                 bool coupling = false)
{
    NodeCompatProfile p;
    p.family = family;
    p.tier = tier;
    p.group = NodeGroup::Preprocess;
    p.terminal = terminal;
    p.couplingNode = coupling;
    return p;
}

const QHash<QString, NodeCompatProfile> &profileTable()
{
    static const QHash<QString, NodeCompatProfile> table = []() {
        QHash<QString, NodeCompatProfile> t;

        // ----- 数据管理（不在 23 模块表内，见文档外规则） -----
        {
            NodeCompatProfile p;
            p.family = SensorFamily::Neutral;
            p.tier = DataTier::None;
            p.group = NodeGroup::DataInput;
            t.insert(QStringLiteral("input.data_input"), p);
        }
        {
            NodeCompatProfile p;
            p.family = SensorFamily::Neutral;
            p.tier = DataTier::None; // 输出 HDF5 时域，视为进入预处理前的桥接
            p.group = NodeGroup::Preprocess;
            t.insert(QStringLiteral("preprocess.format_convert"), p);
        }

        // ----- DAS 1–15（甲方序号 = funcId） -----
        t.insert(QStringLiteral("preprocess.depth_correct"),
                 makePreprocess(SensorFamily::DAS, DataTier::A_TimeDomain));
        t.insert(QStringLiteral("preprocess.depth_time_correct"),
                 makePreprocess(SensorFamily::DAS, DataTier::A_TimeDomain));
        t.insert(QStringLiteral("preprocess.data_crop"),
                 makePreprocess(SensorFamily::DAS, DataTier::A_TimeDomain));
        t.insert(QStringLiteral("preprocess.data_merge"),
                 makePreprocess(SensorFamily::DAS, DataTier::A_TimeDomain));
        t.insert(QStringLiteral("preprocess.das_convert"),
                 makePreprocess(SensorFamily::DAS, DataTier::A_TimeDomain));
        t.insert(QStringLiteral("preprocess.lfdas_extract"),
                 makePreprocess(SensorFamily::DAS, DataTier::A_TimeDomain));
        t.insert(QStringLiteral("preprocess.fk_analysis"),
                 makePreprocess(SensorFamily::DAS, DataTier::B_Spectrum));
        t.insert(QStringLiteral("preprocess.fft_extract"),
                 makePreprocess(SensorFamily::DAS, DataTier::B_Spectrum));
        t.insert(QStringLiteral("preprocess.bandpass_filter"),
                 makePreprocess(SensorFamily::DAS, DataTier::A_TimeDomain));
        t.insert(QStringLiteral("preprocess.downsample"),
                 makePreprocess(SensorFamily::DAS, DataTier::A_TimeDomain));
        t.insert(QStringLiteral("preprocess.fts_extract"),
                 makePreprocess(SensorFamily::DAS, DataTier::B_Spectrum));
        t.insert(QStringLiteral("preprocess.mainfreq_split"),
                 makePreprocess(SensorFamily::DAS, DataTier::B_Spectrum));
        t.insert(QStringLiteral("preprocess.fbe_extract"),
                 makePreprocess(SensorFamily::DAS, DataTier::B_Spectrum));
        t.insert(QStringLiteral("preprocess.moving_avg"),
                 makePreprocess(SensorFamily::DAS, DataTier::A_TimeDomain));
        t.insert(QStringLiteral("preprocess.slow_strain"),
                 makePreprocess(SensorFamily::DAS, DataTier::A_TimeDomain));

        // ----- DTS 16–20 -----
        t.insert(QStringLiteral("preprocess.dts_denoise"),
                 makePreprocess(SensorFamily::DTS, DataTier::A_TimeDomain));
        t.insert(QStringLiteral("preprocess.temp_correct"),
                 makePreprocess(SensorFamily::DTS, DataTier::A_TimeDomain));
        t.insert(QStringLiteral("preprocess.temp_diff"),
                 makePreprocess(SensorFamily::DTS, DataTier::C_Feature, true));
        t.insert(QStringLiteral("preprocess.dtgs_gradient"),
                 makePreprocess(SensorFamily::DTS, DataTier::C_Feature, true));
        t.insert(QStringLiteral("preprocess.baseline_build"),
                 makePreprocess(SensorFamily::DTS, DataTier::A_TimeDomain));

        // ----- DSS 21–23 -----
        t.insert(QStringLiteral("preprocess.strain_disturb"),
                 makePreprocess(SensorFamily::DSS, DataTier::A_TimeDomain));
        t.insert(QStringLiteral("preprocess.temp_coupling"),
                 makePreprocess(SensorFamily::DSS, DataTier::Coupling, false, true));
        t.insert(QStringLiteral("preprocess.baseline_calib"),
                 makePreprocess(SensorFamily::DSS, DataTier::A_TimeDomain));

        // ----- 不在 23 表：物理量转变（虚构参数，按 DTS 时域 A 类衔接） -----
        t.insert(QStringLiteral("preprocess.phys_convert"),
                 makePreprocess(SensorFamily::Neutral, DataTier::A_TimeDomain));

        // ----- 解释分析（虚构模板，接在预处理产物之后） -----
        const QStringList interpretIds = {
            QStringLiteral("interpret.profile_interp"),
            QStringLiteral("interpret.inject_interp"),
            QStringLiteral("interpret.multiphase_diag"),
            QStringLiteral("interpret.frac_interp"),
            QStringLiteral("interpret.interwell_interp"),
            QStringLiteral("interpret.unstable_interp"),
            QStringLiteral("interpret.microseismic_interp"),
            QStringLiteral("interpret.wellbore_interp"),
            QStringLiteral("interpret.welltrack_calc"),
            QStringLiteral("interpret.flowpres_calc"),
            QStringLiteral("interpret.diff_temp_calc"),
            QStringLiteral("interpret.zonal_prod_calc"),
        };
        for (const QString &id : interpretIds)
        {
            NodeCompatProfile p;
            p.family = SensorFamily::Neutral;
            p.tier = DataTier::None;
            p.group = NodeGroup::Interpret;
            t.insert(id, p);
        }

        // ----- 展示类（只消费数据，不再产出给算法） -----
        auto addDisplay = [&t](const QString &id) {
            NodeCompatProfile p;
            p.group = NodeGroup::Display;
            t.insert(id, p);
        };
        addDisplay(QStringLiteral("display.interp_result"));
        addDisplay(QStringLiteral("display.preprocess_result"));
        addDisplay(QStringLiteral("display.multi_result"));
        addDisplay(QStringLiteral("display.wellbore_model"));
        addDisplay(QStringLiteral("display.well_struct"));

        return t;
    }();
    return table;
}

bool fail(QString *reason, const QString &msg)
{
    if (reason)
        *reason = msg;
    return false;
}

/// 数据输入：仅允许接到格式转换（LAS → HDF5）
bool checkDataInputEdge(const NodeMeta &up, const NodeMeta &down, QString *reason)
{
    if (up.typeId != QLatin1String("input.data_input"))
        return true;
    if (down.typeId == QLatin1String("preprocess.format_convert"))
        return true;
    return fail(reason, QObject::tr("数据输入节点只能连接到「数据格式转换」"));
}

/// 格式转换节点：作为下游可接数据输入；作为上游只能接 DAS/DTS/DSS 的 A 类时域
bool checkFormatConvertEdge(const NodeCompatProfile &pu, const NodeCompatProfile &pd,
                            const NodeMeta &up, const NodeMeta &down, QString *reason)
{
    // 连线目标是「格式转换」：允许数据输入等上游接入（白名单见 checkDataInputEdge）
    if (down.typeId == QLatin1String("preprocess.format_convert"))
        return true;

    // 连线的源是「格式转换」：下游必须是 23 表中的 A 类时域预处理
    if (up.typeId != QLatin1String("preprocess.format_convert"))
        return true;

    if (pd.group == NodeGroup::Preprocess && pd.tier == DataTier::A_TimeDomain
        && pd.family != SensorFamily::Neutral)
        return true;

    return fail(reason,
                QObject::tr("格式转换之后请接 DAS/DTS/DSS 的时域预处理节点（A 类）"));
}

/// 解释/展示类：单独规则，不要求 DAS/DTS/DSS 族一致
bool checkInterpretDisplayEdge(const NodeCompatProfile &pu, const NodeCompatProfile &pd,
                               const NodeMeta &up, const NodeMeta &down, QString *reason)
{
    if (pd.group == NodeGroup::Display)
    {
        // 展示节点：允许接预处理/解释产物
        if (up.group == NodeGroup::Preprocess || up.group == NodeGroup::Interpret
            || up.typeId == QLatin1String("input.data_input"))
            return true;
        return fail(reason, QObject::tr("展示节点只能接预处理或解释分析的输出"));
    }

    if (pd.group == NodeGroup::Interpret)
    {
        // 解释节点：上游须为预处理终点（C）、频域结果（B）或解释链，或 phys_convert
        if (up.group == NodeGroup::Interpret)
            return true;
        if (up.typeId == QLatin1String("preprocess.phys_convert"))
            return true;
        if (pu.terminal || pu.tier == DataTier::C_Feature)
            return true;
        if (pu.tier == DataTier::B_Spectrum)
            return true;
        return fail(reason,
                    QObject::tr("解释分析节点应接在预处理特征结果（C 类）或频域结果（B 类）之后"));
    }

    if (pu.group == NodeGroup::Interpret)
    {
        // 解释 → 仅允许解释链或展示
        if (down.group == NodeGroup::Interpret || down.group == NodeGroup::Display)
            return true;
        return fail(reason, QObject::tr("解释分析之后不能再接预处理算法"));
    }

    return true;
}

/// 22 号温度耦合：DTS 时域 或 DSS 21/23 → 22 → 21/23
bool checkCouplingEdge(const NodeCompatProfile &pu, const NodeCompatProfile &pd,
                       const NodeMeta &up, const NodeMeta &down, QString *reason)
{
    if (pd.couplingNode)
    {
        const bool fromDtsA =
            pu.family == SensorFamily::DTS && pu.tier == DataTier::A_TimeDomain;
        const bool fromDssA = up.typeId == QLatin1String("preprocess.strain_disturb")
                              || up.typeId == QLatin1String("preprocess.baseline_calib");
        if (fromDtsA || fromDssA)
            return true;
        return fail(reason,
                    QObject::tr("温度耦合分离须接在 DTS 时域链或 DSS「应变扰动/基线校准」之后"));
    }

    if (pu.couplingNode)
    {
        if (down.typeId == QLatin1String("preprocess.strain_disturb")
            || down.typeId == QLatin1String("preprocess.baseline_calib"))
            return true;
        return fail(reason, QObject::tr("温度耦合分离之后只能接 DSS 应变扰动或基线校准"));
    }

    return true;
}

/// 同族预处理之间的 A/B/C 流转（核心规则）
bool checkPreprocessTierFlow(const NodeCompatProfile &pu, const NodeCompatProfile &pd,
                             const NodeMeta &down, QString *reason)
{
    if (pu.terminal)
        return fail(reason, QObject::tr("特征结果（C 类）之后不能再接算法模块"));

    // 物理量转变（funcId=24，参数仍虚构）：作为「族中立」的时域后处理，可接在任一族 A/B 之后
    if (down.typeId == QLatin1String("preprocess.phys_convert"))
    {
        if (pu.tier == DataTier::A_TimeDomain || pu.tier == DataTier::B_Spectrum)
            return true;
        return fail(reason, QObject::tr("物理量转变须接在时域（A）或频域（B）预处理结果之后"));
    }

    // 跨族：DAS / DTS / DSS 不能互接（耦合节点在上面已处理）
    if (pu.family != SensorFamily::Neutral && pd.family != SensorFamily::Neutral
        && pu.family != pd.family)
    {
        return fail(reason, QObject::tr("数据族不兼容：%1 不能连接到 %2")
                             .arg(toString(pu.family), toString(pd.family)));
    }

    // 格式转换等 Neutral 源：只能启动 A 类
    if (pu.family == SensorFamily::Neutral && pu.tier == DataTier::None)
    {
        if (pd.tier == DataTier::A_TimeDomain)
            return true;
        return fail(reason, QObject::tr("请先将数据接入时域预处理（A 类），再进入频域或特征模块"));
    }

    switch (pd.family)
    {
    case SensorFamily::DAS:
        if (pd.tier == DataTier::A_TimeDomain)
        {
            if (pu.tier == DataTier::A_TimeDomain || pu.family == SensorFamily::Neutral)
                return true;
            return fail(reason, QObject::tr("DAS 时域模块不能接收频域（B 类）或特征（C 类）的输出"));
        }
        if (pd.tier == DataTier::B_Spectrum)
        {
            if (pu.tier == DataTier::A_TimeDomain || pu.tier == DataTier::B_Spectrum)
                return true;
            return fail(reason, QObject::tr("DAS 频域模块只能接时域（A）或频域（B）上游"));
        }
        break;

    case SensorFamily::DTS:
        if (pd.tier == DataTier::A_TimeDomain)
        {
            if (pu.tier == DataTier::A_TimeDomain || pu.family == SensorFamily::Neutral)
                return true;
            return fail(reason, QObject::tr("DTS 时域模块不能接收特征（C 类）输出"));
        }
        if (pd.tier == DataTier::C_Feature)
        {
            if (pu.tier == DataTier::A_TimeDomain)
                return true;
            return fail(reason, QObject::tr("DTS 特征模块（差分/梯度）只能接 DTS 时域链之后"));
        }
        break;

    case SensorFamily::DSS:
        if (pd.tier == DataTier::A_TimeDomain)
        {
            if (pu.tier == DataTier::A_TimeDomain || pu.tier == DataTier::Coupling
                || pu.family == SensorFamily::Neutral)
                return true;
            return fail(reason, QObject::tr("DSS 时域模块不能接收 DAS/DTS 常规模块的输出"));
        }
        break;

    default:
        break;
    }

    return fail(reason, QObject::tr("不支持的预处理衔接"));
}

} // namespace

QString toString(SensorFamily f)
{
    switch (f)
    {
    case SensorFamily::DAS:
        return QObject::tr("DAS");
    case SensorFamily::DTS:
        return QObject::tr("DTS");
    case SensorFamily::DSS:
        return QObject::tr("DSS");
    case SensorFamily::Neutral:
    default:
        return QObject::tr("通用");
    }
}

QString toString(DataTier t)
{
    switch (t)
    {
    case DataTier::A_TimeDomain:
        return QObject::tr("时域 A");
    case DataTier::B_Spectrum:
        return QObject::tr("频域 B");
    case DataTier::C_Feature:
        return QObject::tr("特征 C");
    case DataTier::Coupling:
        return QObject::tr("耦合");
    case DataTier::None:
    default:
        return QObject::tr("无");
    }
}

NodeCompatProfile compatProfileForTypeId(const QString &typeId)
{
    const auto it = profileTable().constFind(typeId);
    if (it != profileTable().constEnd())
        return it.value();
    NodeCompatProfile p;
    p.group = NodeGroup::Preprocess;
    return p;
}

bool validateWorkflowCompatGraph(
    const QList<EdgeInstance> &edges,
    const std::function<QString(const QString &instanceId)> &typeIdOfNode,
    const std::function<const NodeMeta *(const QString &typeId)> &metaOfType,
    QString *reason)
{
    for (const EdgeInstance &e : edges)
    {
        const QString fromType = typeIdOfNode(e.fromNode);
        const NodeCompatProfile pu = compatProfileForTypeId(fromType);
        if (!pu.terminal)
            continue;

        const NodeMeta *fm = metaOfType(fromType);
        const QString name = fm ? fm->displayName : fromType;
        return fail(reason,
                    QObject::tr("节点「%1」为特征终点（C 类），不能再连出到下游算法").arg(name));
    }

    for (const EdgeInstance &e : edges)
    {
        const QString fromType = typeIdOfNode(e.fromNode);
        const QString toType = typeIdOfNode(e.toNode);
        const NodeMeta *upMeta = metaOfType(fromType);
        const NodeMeta *downMeta = metaOfType(toType);
        if (!upMeta || !downMeta)
            continue;
        if (!isAlgorithmEdgeCompatible(*upMeta, *downMeta, reason))
            return false;
    }
    return true;
}

bool isAlgorithmEdgeCompatible(const NodeMeta &upstream, const NodeMeta &downstream,
                               QString *reason)
{
    const NodeCompatProfile pu = compatProfileForTypeId(upstream.typeId);
    const NodeCompatProfile pd = compatProfileForTypeId(downstream.typeId);

    if (!checkDataInputEdge(upstream, downstream, reason))
        return false;

    if (!checkFormatConvertEdge(pu, pd, upstream, downstream, reason))
        return false;

    if (!checkCouplingEdge(pu, pd, upstream, downstream, reason))
        return false;

    if (!checkInterpretDisplayEdge(pu, pd, upstream, downstream, reason))
        return false;

    // 预处理 ↔ 预处理
    if (pu.group == NodeGroup::Preprocess && pd.group == NodeGroup::Preprocess)
        return checkPreprocessTierFlow(pu, pd, downstream, reason);

    // 预处理 → 解释/展示 已在 checkInterpretDisplayEdge 处理
    if (pu.group == NodeGroup::Preprocess
        && (pd.group == NodeGroup::Interpret || pd.group == NodeGroup::Display))
        return true;

    return true;
}

} // namespace processing
