#ifndef PARAM_RANGE_PAIR_H
#define PARAM_RANGE_PAIR_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace processing
{

    /// 节点参数「最小~最大」范围对（属性面板双 Spinbox，运行时可同步为 legacy 数组）
    struct ParamRangePairSpec
    {
        QString legacyKey;
        QString minKey;
        QString maxKey;
        QString rowLabel;
        double defaultMin = 0.0;
        double defaultMax = 1000.0;
        bool maxLimitFsHalf = false;
        bool legacyIsString = false;
        bool minExclusive = false; ///< 最小值须严格大于 defaultMin（如带通低频 > 0）
    };

    QList<ParamRangePairSpec> paramRangePairsForTypeId(const QString &typeId);

    QStringList paramRangeMaxKeys(const QList<ParamRangePairSpec> &specs);

    bool migrateParamRangePairs(QVariantMap &params, const QList<ParamRangePairSpec> &specs);

    void clampParamRangePairs(QVariantMap &params, const QList<ParamRangePairSpec> &specs);

    bool validateParamRangePairs(const QVariantMap &params,
                                const QList<ParamRangePairSpec> &specs,
                                QString *errorOut);

    void syncLegacyRangeArrays(QVariantMap &params, const QList<ParamRangePairSpec> &specs);

} // namespace processing

#endif // PARAM_RANGE_PAIR_H
