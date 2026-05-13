#include "DemoNodes.h"
#include "NodeRegistry.h"

namespace processing
{
    namespace demo
    {

        namespace
        {

            NodeMeta makeLasInputMeta()
            {
                NodeMeta m;
                m.typeId = "input.las";
                m.displayName = QObject::tr("LAS 数据输入");
                m.category = QObject::tr("数据输入");
                m.group = NodeGroup::DataInput;
                m.kind = NodeKind::PureOutput;
                m.outputs = {{"out", QObject::tr("数据"), DataFormat::HDF5}};
                m.defaultParams.insert("path", "");
                m.description = QObject::tr("从 LAS 文件读取测井曲线，转换为内部 HDF5。");
                m.order = 1;
                m.externalProcess = true; // 真实应用中应为 true
                return m;
            }

            NodeMeta makeDespikeMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.despike";
                m.displayName = QObject::tr("去毛刺");
                m.category = QObject::tr("DAS 处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.inputs = {{"in", QObject::tr("输入"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("输出"), DataFormat::HDF5}};
                m.defaultParams.insert("threshold", 3.0);
                m.description = QObject::tr("基于阈值滤除尖峰噪声。");
                m.order = 2;
                return m;
            }

            NodeMeta makeBandpassMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.bandpass";
                m.displayName = QObject::tr("带通滤波");
                m.category = QObject::tr("DAS 处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.inputs = {{"in", QObject::tr("输入"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("输出"), DataFormat::HDF5}};
                m.defaultParams.insert("lowCut", 10.0);
                m.defaultParams.insert("highCut", 200.0);
                m.description = QObject::tr("Butterworth 带通滤波器。");
                m.order = 3;
                return m;
            }

            NodeMeta makeLinePlotMeta()
            {
                NodeMeta m;
                m.typeId = "display.lineplot";
                m.displayName = QObject::tr("折线图");
                m.category = QObject::tr("可视化");
                m.group = NodeGroup::Display;
                m.kind = NodeKind::InOut;
                m.inputs = {{"in", QObject::tr("数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("透传"), DataFormat::HDF5}};
                m.description = QObject::tr("将上游通道数据以折线图展示，可导出 PNG/CSV；同时透传数据给下游节点。");
                m.order = 4;
                m.externalProcess = false;
                return m;
            }

            NodeMeta makeMergeMeta()
            {
                NodeMeta m;
                m.typeId = "display.merge";
                m.displayName = QObject::tr("多结果汇总");
                m.category = QObject::tr("可视化");
                m.group = NodeGroup::Display;
                m.kind = NodeKind::PureInput;
                m.inputs = {
                    {"in1", QObject::tr("结果 1"), DataFormat::HDF5},
                    {"in2", QObject::tr("结果 2"), DataFormat::HDF5},
                    {"in3", QObject::tr("结果 3"), DataFormat::HDF5}};
                m.description = QObject::tr("合并多个上游结果到一个汇总报告。");
                m.order = 5;
                m.externalProcess = false;
                return m;
            }

        } // namespace

        void registerDemoNodes()
        {
            auto &reg = NodeRegistry::instance();
            if (reg.hasType("input.las"))
                return; // 已注册，幂等

            reg.registerType(makeLasInputMeta(), [m = makeLasInputMeta()]
                             { return std::make_shared<DemoNodeBase>(m); });
            reg.registerType(makeDespikeMeta(), [m = makeDespikeMeta()]
                             { return std::make_shared<DemoNodeBase>(m); });
            reg.registerType(makeBandpassMeta(), [m = makeBandpassMeta()]
                             { return std::make_shared<DemoNodeBase>(m); });
            reg.registerType(makeLinePlotMeta(), [m = makeLinePlotMeta()]
                             { return std::make_shared<DemoNodeBase>(m); });
            reg.registerType(makeMergeMeta(), [m = makeMergeMeta()]
                             { return std::make_shared<DemoNodeBase>(m); });
        }

    }
} // namespace processing::demo
