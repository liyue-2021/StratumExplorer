#include "ParamRangePair.h"

#include <QObject>
#include <QtMath>

namespace processing
{

namespace
{

bool readPairFromVariant(const QVariant &val, double *a, double *b)
{
    if (val.typeId() == QMetaType::QVariantList)
    {
        const QVariantList list = val.toList();
        if (list.size() < 2)
            return false;
        bool okA = false;
        bool okB = false;
        *a = list.at(0).toDouble(&okA);
        *b = list.at(1).toDouble(&okB);
        return okA && okB;
    }
    const QString text = val.toString().trimmed();
    if (!text.contains(QLatin1Char(',')))
        return false;
    const QStringList parts = text.split(QLatin1Char(','), Qt::SkipEmptyParts);
    if (parts.size() < 2)
        return false;
    bool okA = false;
    bool okB = false;
    *a = parts.at(0).trimmed().toDouble(&okA);
    *b = parts.at(1).trimmed().toDouble(&okB);
    return okA && okB;
}

double upperLimitForSpec(const QVariantMap &params, const ParamRangePairSpec &spec)
{
    if (spec.maxLimitFsHalf)
    {
        const double fs = qMax(1e-9, params.value(QStringLiteral("fs")).toDouble());
        return fs / 2.0;
    }
    return 1e12;
}

double lowerLimitForSpec(const ParamRangePairSpec &spec)
{
    return spec.minExclusive ? spec.defaultMin + 1e-9 : spec.defaultMin;
}

} // namespace

QList<ParamRangePairSpec> paramRangePairsForTypeId(const QString &typeId)
{
    if (typeId == QLatin1String("preprocess.data_crop"))
    {
        return {{QString(),
                 QStringLiteral("depth_min"),
                 QStringLiteral("depth_max"),
                 QObject::tr("深度范围"),
                 0.0,
                 1000.0},
                {QString(),
                 QStringLiteral("time_min"),
                 QStringLiteral("time_max"),
                 QObject::tr("时间范围"),
                 0.0,
                 1000.0}};
    }
    if (typeId == QLatin1String("preprocess.das_convert"))
    {
        return {{QString(),
                 QStringLiteral("norm_min"),
                 QStringLiteral("norm_max"),
                 QObject::tr("归一化范围"),
                 0.0,
                 1.0}};
    }
    if (typeId == QLatin1String("preprocess.lfdas_extract"))
    {
        return {{QString(),
                 QStringLiteral("f_low"),
                 QStringLiteral("f_high"),
                 QObject::tr("频带范围"),
                 1.0,
                 50.0,
                 true}};
    }
    if (typeId == QLatin1String("preprocess.bandpass_filter"))
    {
        return {{QString(),
                 QStringLiteral("f_low"),
                 QStringLiteral("f_high"),
                 QObject::tr("频带范围"),
                 0.0,
                 200.0,
                 true,
                 false,
                 true}};
    }
    if (typeId == QLatin1String("preprocess.data_merge"))
    {
        return {{QStringLiteral("channel_range"),
                 QStringLiteral("channel_min"),
                 QStringLiteral("channel_max"),
                 QObject::tr("通道范围"),
                 0.0,
                 0.0}};
    }
    if (typeId == QLatin1String("preprocess.fk_analysis"))
    {
        return {{QStringLiteral("depth_range"),
                 QStringLiteral("depth_min"),
                 QStringLiteral("depth_max"),
                 QObject::tr("深度范围"),
                 0.0,
                 1000.0},
                {QStringLiteral("time_range"),
                 QStringLiteral("time_min"),
                 QStringLiteral("time_max"),
                 QObject::tr("时间范围"),
                 0.0,
                 1000.0}};
    }
    if (typeId == QLatin1String("preprocess.fft_extract")
        || typeId == QLatin1String("preprocess.fts_extract"))
    {
        return {{QStringLiteral("freq_range"),
                 QStringLiteral("f_min"),
                 QStringLiteral("f_max"),
                 QObject::tr("频率范围"),
                 0.0,
                 500.0,
                 true}};
    }
    if (typeId == QLatin1String("preprocess.mainfreq_split"))
    {
        return {{QStringLiteral("depth_range"),
                 QStringLiteral("depth_min"),
                 QStringLiteral("depth_max"),
                 QObject::tr("深度范围"),
                 0.0,
                 1000.0},
                {QStringLiteral("time_range"),
                 QStringLiteral("time_min"),
                 QStringLiteral("time_max"),
                 QObject::tr("时间范围"),
                 0.0,
                 1000.0},
                {QStringLiteral("manual_bands"),
                 QStringLiteral("band_min"),
                 QStringLiteral("band_max"),
                 QObject::tr("人工频段"),
                 0.0,
                 500.0,
                 true}};
    }
    if (typeId == QLatin1String("preprocess.fbe_extract"))
    {
        return {{QStringLiteral("freq_bands"),
                 QStringLiteral("band_min"),
                 QStringLiteral("band_max"),
                 QObject::tr("分频频段"),
                 0.0,
                 100.0,
                 true,
                 true}};
    }
    if (typeId == QLatin1String("preprocess.baseline_build"))
    {
        return {{QStringLiteral("ref_time_range"),
                 QStringLiteral("ref_time_min"),
                 QStringLiteral("ref_time_max"),
                 QObject::tr("参考时间范围"),
                 0.0,
                 1000.0},
                {QStringLiteral("depth_range"),
                 QStringLiteral("depth_min"),
                 QStringLiteral("depth_max"),
                 QObject::tr("深度范围"),
                 0.0,
                 1000.0}};
    }
    if (typeId == QLatin1String("preprocess.baseline_calib"))
    {
        return {{QStringLiteral("ref_segment"),
                 QStringLiteral("ref_seg_min"),
                 QStringLiteral("ref_seg_max"),
                 QObject::tr("参考段范围"),
                 0.0,
                 1000.0}};
    }
    return {};
}

QStringList paramRangeMaxKeys(const QList<ParamRangePairSpec> &specs)
{
    QStringList keys;
    for (const ParamRangePairSpec &s : specs)
        keys.append(s.maxKey);
    return keys;
}

bool migrateParamRangePairs(QVariantMap &params, const QList<ParamRangePairSpec> &specs)
{
    bool changed = false;
    for (const ParamRangePairSpec &spec : specs)
    {
        if (params.contains(spec.minKey) && params.contains(spec.maxKey))
            continue;

        double a = spec.defaultMin;
        double b = spec.defaultMax;
        if (!spec.legacyKey.isEmpty() && params.contains(spec.legacyKey))
        {
            if (!readPairFromVariant(params.value(spec.legacyKey), &a, &b)
                && spec.maxLimitFsHalf)
            {
                const double fs = params.value(QStringLiteral("fs"), 1000.0).toDouble();
                a = spec.defaultMin;
                b = fs > 0.0 ? fs / 2.0 : spec.defaultMax;
            }
        }
        else if (spec.maxLimitFsHalf)
        {
            const double fs = params.value(QStringLiteral("fs"), 1000.0).toDouble();
            b = fs > 0.0 ? fs / 2.0 : spec.defaultMax;
        }

        params.insert(spec.minKey, a);
        params.insert(spec.maxKey, b);
        if (!spec.legacyKey.isEmpty())
            params.remove(spec.legacyKey);
        changed = true;
    }
    return changed;
}

void clampParamRangePairs(QVariantMap &params, const QList<ParamRangePairSpec> &specs)
{
    for (const ParamRangePairSpec &spec : specs)
    {
        if (!params.contains(spec.minKey) || !params.contains(spec.maxKey))
            continue;
        const double upper = upperLimitForSpec(params, spec);
        const double lower = lowerLimitForSpec(spec);
        double mn = params.value(spec.minKey).toDouble();
        double mx = params.value(spec.maxKey).toDouble();
        mn = qBound(lower, mn, upper);
        mx = qBound(mn + 1e-6, mx, upper);
        params.insert(spec.minKey, mn);
        params.insert(spec.maxKey, mx);
    }
}

bool validateParamRangePairs(const QVariantMap &params,
                            const QList<ParamRangePairSpec> &specs,
                            QString *errorOut)
{
    for (const ParamRangePairSpec &spec : specs)
    {
        if (!params.contains(spec.minKey) || !params.contains(spec.maxKey))
            continue;
        const double upper = upperLimitForSpec(params, spec);
        const double lower = lowerLimitForSpec(spec);
        const double mn = params.value(spec.minKey).toDouble();
        const double mx = params.value(spec.maxKey).toDouble();
        if (mn < lower - 1e-9 || mx > upper + 1e-6 || mn >= mx)
        {
            if (errorOut)
            {
                if (spec.maxLimitFsHalf && spec.minExclusive)
                    *errorOut = QObject::tr("%1须满足 0 < 最小 < 最大 ≤ 采样率/2")
                                     .arg(spec.rowLabel);
                else if (spec.maxLimitFsHalf)
                    *errorOut =
                        QObject::tr("%1须满足 0 ≤ 最小 < 最大 ≤ 采样率/2").arg(spec.rowLabel);
                else
                    *errorOut = QObject::tr("%1须满足 最小 < 最大").arg(spec.rowLabel);
            }
            return false;
        }
    }
    return true;
}

void syncLegacyRangeArrays(QVariantMap &params, const QList<ParamRangePairSpec> &specs)
{
    for (const ParamRangePairSpec &spec : specs)
    {
        if (spec.legacyKey.isEmpty() || !params.contains(spec.minKey)
            || !params.contains(spec.maxKey))
            continue;
        const double a = params.value(spec.minKey).toDouble();
        const double b = params.value(spec.maxKey).toDouble();
        if (spec.legacyIsString)
        {
            params.insert(spec.legacyKey,
                          QStringLiteral("%1,%2").arg(a, 0, 'g', 12).arg(b, 0, 'g', 12));
        }
        else
        {
            QVariantList list;
            list.append(a);
            list.append(b);
            params.insert(spec.legacyKey, list);
        }
    }
}

} // namespace processing
