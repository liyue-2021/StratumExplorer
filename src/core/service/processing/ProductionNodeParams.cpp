#include "ProductionNodeParams.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHash>
#include <QLoggingCategory>

namespace processing::production
{

    namespace
    {

        Q_LOGGING_CATEGORY(lcNodeParams, "processing.nodeParams")

        struct ClientParamSpec
        {
            QVariantMap defaults;
            QMap<QString, QString> labels;
            QStringList order;
            bool fictional = false;
            bool hidePropertyPanel = false;
            QString configDialogId;
        };

        QVariant jsonToVariant(const QJsonValue &v, const QString &type)
        {
            if (type == QLatin1String("array"))
            {
                QVariantList list;
                if (v.isArray())
                {
                    for (const QJsonValue &item : v.toArray())
                    {
                        if (item.isDouble())
                            list.append(item.toDouble());
                        else
                            list.append(item.toVariant());
                    }
                }
                return list;
            }
            if (type == QLatin1String("bool"))
                return v.toBool();
            if (type == QLatin1String("int"))
                return v.toInt();
            if (type == QLatin1String("float"))
                return v.toDouble();
            return v.toString();
        }

        QByteArray loadClientParamsJson()
        {
            QFile res(QStringLiteral(":/processing/node_client_params.json"));
            if (res.open(QIODevice::ReadOnly))
                return res.readAll();

            // 静态库 QRC 未链入时的兜底：exe 同目录或源码目录
            const QString appDir = QCoreApplication::applicationDirPath();
            const QStringList candidates = {
                appDir + QStringLiteral("/node_client_params.json"),
                appDir + QStringLiteral("/config/node_client_params.json"),
                appDir + QStringLiteral("/../src/core/service/processing/node_client_params.json"),
            };
            for (const QString &path : candidates)
            {
                QFile f(path);
                if (f.open(QIODevice::ReadOnly))
                {
                    qCWarning(lcNodeParams) << "loaded node_client_params.json from" << path;
                    return f.readAll();
                }
            }

            qCWarning(lcNodeParams) << "failed to load node_client_params.json (resource and file fallback)";
            return {};
        }

        const QHash<QString, ClientParamSpec> &clientParamTable()
        {
            static QHash<QString, ClientParamSpec> table = []()
            {
                Q_INIT_RESOURCE(node_client_params);
                QHash<QString, ClientParamSpec> t;
                const QByteArray raw = loadClientParamsJson();
                if (raw.isEmpty())
                    return t;

                const QJsonDocument doc = QJsonDocument::fromJson(raw);
                if (!doc.isObject())
                {
                    qCWarning(lcNodeParams) << "node_client_params.json is not a JSON object";
                    return t;
                }

                const QJsonObject root = doc.object();
                for (auto it = root.constBegin(); it != root.constEnd(); ++it)
                {
                    if (it.key().startsWith(QLatin1Char('_')))
                        continue;
                    if (!it.value().isObject())
                        continue;

                    const QJsonObject nodeObj = it.value().toObject();
                    if (!nodeObj.contains(QStringLiteral("params")))
                        continue;
                    const QJsonArray orderArr = nodeObj.value(QStringLiteral("order")).toArray();
                    const QJsonObject paramsObj = nodeObj.value(QStringLiteral("params")).toObject();

                    ClientParamSpec spec;
                    for (const QJsonValue &ov : orderArr)
                        spec.order.append(ov.toString());

                    for (auto pit = paramsObj.constBegin(); pit != paramsObj.constEnd(); ++pit)
                    {
                        if (!pit.value().isObject())
                            continue;
                        const QJsonObject p = pit.value().toObject();
                        const QString key = pit.key();
                        const QString type = p.value(QStringLiteral("type")).toString();
                        const QJsonValue defVal = p.value(QStringLiteral("default"));
                        spec.defaults.insert(key, jsonToVariant(defVal, type));
                        spec.labels.insert(key, p.value(QStringLiteral("label")).toString());
                    }

                    spec.fictional = nodeObj.value(QStringLiteral("fictional")).toBool(false);
                    spec.hidePropertyPanel =
                        nodeObj.value(QStringLiteral("propertyPanel")).toString() == QLatin1String("none");
                    spec.configDialogId = nodeObj.value(QStringLiteral("configDialog")).toString();
                    t.insert(it.key(), spec);
                }

                qCInfo(lcNodeParams) << "loaded client param specs for" << t.size() << "node types";
                return t;
            }();
            return table;
        }

    } // namespace

    void applyClientParams(NodeMeta &meta)
    {
        const auto it = clientParamTable().constFind(meta.typeId);
        if (it == clientParamTable().constEnd())
            return;

        const ClientParamSpec &spec = it.value();
        meta.defaultParams = spec.defaults;
        meta.paramLabels = spec.labels;
        meta.paramOrder = spec.order;
        meta.clientParamsFictional = spec.fictional;
        meta.hidePropertyPanel = spec.hidePropertyPanel;
        meta.configDialogId = spec.configDialogId;

        if (meta.externalProcess && !meta.hidePropertyPanel)
            meta.paramLabels.insert(QStringLiteral("exePath"), QObject::tr("程序路径"));
    }

    NodeMeta finalizeMeta(NodeMeta meta)
    {
        applyClientParams(meta);
        return meta;
    }

    QVariantMap mergeWithClientDefaults(const QString &typeId, const QVariantMap &saved)
    {
        const auto it = clientParamTable().constFind(typeId);
        if (it == clientParamTable().constEnd())
            return saved;

        QVariantMap merged = it.value().defaults;
        for (auto sit = saved.constBegin(); sit != saved.constEnd(); ++sit)
            merged.insert(sit.key(), sit.value());
        return merged;
    }

    bool isFictionalClientParams(const QString &typeId)
    {
        const auto it = clientParamTable().constFind(typeId);
        return it != clientParamTable().constEnd() && it.value().fictional;
    }

} // namespace processing::production
