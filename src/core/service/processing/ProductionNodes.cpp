// =============================================================================
//  ProductionNodes.cpp
//  作者：工程师 ly（自动生成）
//
//  基于甲方节点规格文档（数据预处理模块输入输出说明-20260509-v1.xlsx）
//  生成的节点元数据和注册代码。
//
//  属性参数：23 个预处理节点来自 Excel（真实）；另有 19 个节点参数为行业模板、
//  暂为虚构（format_convert、phys_convert、interpret×12、display×5），见
//  node_client_params.json 中 fictional=true 及 depend/oilPro_42个节点_属性面板参数表。
//
//  节点分组：
//    - Preprocess（预处理小组件）：数据格式转换、深度矫正、时间矫正等
//    - Interpret（解释分析类）：产出剖面解释、注入剖面解释等
//    - Display（展示类）：结果展示、模型展示等
// =============================================================================
#include "ProductionNodes.h"
#include "ProductionNodeParams.h"
#include "ExternalProcessNode.h"
#include "NodeRegistry.h"

namespace processing
{
    namespace production
    {

        namespace
        {

            // ============================================================================
            // 辅助函数：创建节点元数据
            // ============================================================================

            // 数据管理 - 数据输入（右侧属性面板选 LAS；无双击弹窗）
            NodeMeta makeDataInputMeta()
            {
                NodeMeta m;
                m.typeId = "input.data_input";
                m.displayName = QObject::tr("数据输入");
                m.category = QObject::tr("数据输入");
                m.group = NodeGroup::DataInput;
                m.kind = NodeKind::PureOutput;
                m.funcId = -1;
                m.funcName = QString();
                m.inputs = {};
                m.outputs = {{"out", QObject::tr("数据文件"), DataFormat::LAS}};
                m.description = QObject::tr("选择 LAS 测井文件作为工作流输入，可多选。");
                m.order = 0;
                m.externalProcess = false;
                return finalizeMeta(m);
            }

            // 预处理组 - 数据格式转换（参数见 node_client_params.json；右侧属性面板）
            NodeMeta makeFormatConvertMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.format_convert";
                m.displayName = QObject::tr("数据格式转换");
                m.category = QObject::tr("格式转换");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 0;
                m.funcName = "func_format_convert";
                m.inputs = {{"in", QObject::tr("输入"), DataFormat::LAS}};
                m.outputs = {{"out", QObject::tr("输出"), DataFormat::HDF5}};
                m.description = QObject::tr("数据格式转换，将各种输入格式转换为统一的 HDF5 格式。");
                m.order = 1;
                // 格式转换走应用内后端服务，不启动外部 EXE
                m.externalProcess = false;
                return finalizeMeta(m);
            }

            // 预处理组 - 深度矫正 (funcId=1)
            NodeMeta makeDepthCorrectMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.depth_correct";
                m.displayName = QObject::tr("深度矫正");
                m.category = QObject::tr("预处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 1;
                m.funcName = "func_depth_correct";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("输出数据"), DataFormat::HDF5}};
                m.description = QObject::tr("基于参考深度文件，对测井数据进行深度校准，支持插值修正。");
                m.order = 2;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - 时间矫正 (funcId=2)
            NodeMeta makeDepthTimeCorrectMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.depth_time_correct";
                m.displayName = QObject::tr("时间矫正");
                m.category = QObject::tr("预处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 2;
                m.funcName = "func_depth_time_correct";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("输出数据"), DataFormat::HDF5}};
                m.description = QObject::tr("对数据的时间轴进行偏移、缩放矫正，确保时深匹配。");
                m.order = 3;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - 数据裁剪 (funcId=3)
            NodeMeta makeDataCropMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.data_crop";
                m.displayName = QObject::tr("数据裁剪");
                m.category = QObject::tr("预处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 3;
                m.funcName = "func_data_crop";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("输出数据"), DataFormat::HDF5}};
                m.description = QObject::tr("根据指定的深度、时间范围，剪裁无效数据，保留有效区间。");
                m.order = 4;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - 数据合并 (funcId=4)
            NodeMeta makeDataMergeMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.data_merge";
                m.displayName = QObject::tr("数据合并");
                m.category = QObject::tr("预处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 4;
                m.funcName = "func_data_merge";
                m.inputs = {{"in", QObject::tr("待合并数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("合并数据"), DataFormat::HDF5}};
                m.description = QObject::tr("将多个文件按指定轴（深度/时间）拼接合并，支持通道筛选。");
                m.order = 5;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - DAS数据转换 (funcId=5)
            NodeMeta makeDasConvertMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.das_convert";
                m.displayName = QObject::tr("DAS数据转换");
                m.category = QObject::tr("DAS处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 5;
                m.funcName = "func_das_convert";
                m.inputs = {{"in", QObject::tr("原始DAS数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("转换后数据"), DataFormat::HDF5}};
                m.description = QObject::tr("DAS（分布式声波传感）数据格式转换处理。");
                m.order = 6;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - LF-DAS提取 (funcId=6)
            NodeMeta makeLfdasExtractMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.lfdas_extract";
                m.displayName = QObject::tr("LF-DAS提取");
                m.category = QObject::tr("DAS处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 6;
                m.funcName = "func_lfdas_extract";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("提取结果"), DataFormat::HDF5}};
                m.description = QObject::tr("低频 DAS 信号提取。");
                m.order = 7;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - F-K分析 (funcId=7)
            NodeMeta makeFkAnalysisMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.fk_analysis";
                m.displayName = QObject::tr("F-K分析");
                m.category = QObject::tr("DAS处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 7;
                m.funcName = "func_fk_analysis";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("分析结果"), DataFormat::HDF5}};
                m.description = QObject::tr("频率-波数（F-K）分析。");
                m.order = 8;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - FFT提取 (funcId=8)
            NodeMeta makeFftExtractMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.fft_extract";
                m.displayName = QObject::tr("FFT提取");
                m.category = QObject::tr("DAS处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 8;
                m.funcName = "func_fft_extract";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("频谱数据"), DataFormat::HDF5}};
                m.description = QObject::tr("快速傅里叶变换（FFT）频谱提取。");
                m.order = 9;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - 带通滤波 (funcId=9)
            NodeMeta makeBandpassFilterMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.bandpass_filter";
                m.displayName = QObject::tr("带通滤波");
                m.category = QObject::tr("DAS处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 9;
                m.funcName = "func_bandpass_filter";
                m.inputs = {{"in", QObject::tr("输入信号"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("滤波结果"), DataFormat::HDF5}};
                m.description = QObject::tr("Butterworth 带通滤波器，对 DAS 信号做频率筛选。");
                m.order = 10;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - 降采样 (funcId=10)
            NodeMeta makeDownsampleMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.downsample";
                m.displayName = QObject::tr("降采样");
                m.category = QObject::tr("DAS处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 10;
                m.funcName = "func_downsample";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("降采样后数据"), DataFormat::HDF5}};
                m.description = QObject::tr("数据降采样，减少数据量。");
                m.order = 11;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - FTS提取 (funcId=11)
            NodeMeta makeFtsExtractMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.fts_extract";
                m.displayName = QObject::tr("FTS提取");
                m.category = QObject::tr("DAS处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 11;
                m.funcName = "func_fts_extract";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("FTS结果"), DataFormat::HDF5}};
                m.description = QObject::tr("傅里叶变换谱（FTS）提取。");
                m.order = 12;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - 主频切分 (funcId=12)
            NodeMeta makeMainfreqSplitMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.mainfreq_split";
                m.displayName = QObject::tr("主频切分");
                m.category = QObject::tr("DAS处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 12;
                m.funcName = "func_mainfreq_split";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("切分结果"), DataFormat::HDF5}};
                m.description = QObject::tr("按主频率对信号进行切分处理。");
                m.order = 13;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - FBE提取 (funcId=13)
            NodeMeta makeFbeExtractMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.fbe_extract";
                m.displayName = QObject::tr("FBE提取");
                m.category = QObject::tr("DAS处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 13;
                m.funcName = "func_fbe_extract";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("FBE结果"), DataFormat::HDF5}};
                m.description = QObject::tr("FBE（Frequency Band Energy）频带能量提取。");
                m.order = 14;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - 滑动平均 (funcId=14)
            NodeMeta makeMovingAvgMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.moving_avg";
                m.displayName = QObject::tr("滑动平均");
                m.category = QObject::tr("DAS处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 14;
                m.funcName = "func_moving_avg";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("平滑结果"), DataFormat::HDF5}};
                m.description = QObject::tr("滑动平均/中值滤波，平滑数据。");
                m.order = 15;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - 慢应变 (funcId=15)
            NodeMeta makeSlowStrainMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.slow_strain";
                m.displayName = QObject::tr("慢应变");
                m.category = QObject::tr("DTS处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 15;
                m.funcName = "func_slow_strain";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("慢应变结果"), DataFormat::HDF5}};
                m.description = QObject::tr("慢应变信号提取。");
                m.order = 16;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - DTS去噪 (funcId=16)
            NodeMeta makeDtsDenoiseMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.dts_denoise";
                m.displayName = QObject::tr("DTS去噪");
                m.category = QObject::tr("DTS处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 16;
                m.funcName = "func_dts_denoise";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("去噪结果"), DataFormat::HDF5}};
                m.description = QObject::tr("DTS（分布式温度传感）数据去噪处理。");
                m.order = 17;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - 温度矫正 (funcId=17)
            NodeMeta makeTempCorrectMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.temp_correct";
                m.displayName = QObject::tr("温度矫正");
                m.category = QObject::tr("DTS处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 17;
                m.funcName = "func_temp_correct";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("矫正结果"), DataFormat::HDF5}};
                m.description = QObject::tr("温度数据矫正处理。");
                m.order = 18;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - 温度差分 (funcId=18)
            NodeMeta makeTempDiffMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.temp_diff";
                m.displayName = QObject::tr("温度差分");
                m.category = QObject::tr("DTS处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 18;
                m.funcName = "func_temp_diff";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("差分结果"), DataFormat::HDF5}};
                m.description = QObject::tr("温度差分计算。");
                m.order = 19;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - DTGS梯度 (funcId=19)
            NodeMeta makeDtgsGradientMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.dtgs_gradient";
                m.displayName = QObject::tr("DTGS梯度");
                m.category = QObject::tr("DTS处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 19;
                m.funcName = "func_dtgs_gradient";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("梯度结果"), DataFormat::HDF5}};
                m.description = QObject::tr("DTGS（分布式温度梯度传感）梯度计算。");
                m.order = 20;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - 基线构建 (funcId=20)
            NodeMeta makeBaselineBuildMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.baseline_build";
                m.displayName = QObject::tr("基线构建");
                m.category = QObject::tr("DTS处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 20;
                m.funcName = "func_baseline_build";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("基线数据"), DataFormat::HDF5}};
                m.description = QObject::tr("构建温度/应变基线。");
                m.order = 21;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - 应变扰动 (funcId=21)
            NodeMeta makeStrainDisturbMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.strain_disturb";
                m.displayName = QObject::tr("应变扰动");
                m.category = QObject::tr("DTS处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 21;
                m.funcName = "func_strain_disturb";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("扰动结果"), DataFormat::HDF5}};
                m.description = QObject::tr("应变扰动分析处理。");
                m.order = 22;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - 温度耦合分离 (funcId=22) - DSS温度耦合分离
            NodeMeta makeTempCouplingMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.temp_coupling";
                m.displayName = QObject::tr("DSS温度耦合分离");
                m.category = QObject::tr("DTS处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 22;
                m.funcName = "func_dss_temp_couple_separate";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("分离结果"), DataFormat::HDF5}};
                m.description = QObject::tr("利用DTS温度数据，剥离温度诱导应变，得纯机械应变。");
                m.order = 23;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - 基线校准 (funcId=23)
            NodeMeta makeBaselineCalibMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.baseline_calib";
                m.displayName = QObject::tr("基线校准");
                m.category = QObject::tr("DTS处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 23;
                m.funcName = "func_baseline_calib";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("校准结果"), DataFormat::HDF5}};
                m.description = QObject::tr("基线校准处理。");
                m.order = 24;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 预处理组 - 物理量转变（⚠ 属性参数暂为虚构）(funcId=24)
            NodeMeta makePhysConvertMeta()
            {
                NodeMeta m;
                m.typeId = "preprocess.phys_convert";
                m.displayName = QObject::tr("物理量转变");
                m.category = QObject::tr("DTS处理");
                m.group = NodeGroup::Preprocess;
                m.kind = NodeKind::InOut;
                m.funcId = 24;
                m.funcName = "func_phys_convert";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("转换结果"), DataFormat::HDF5}};
                m.description = QObject::tr("物理量单位转换处理。");
                m.order = 25;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // ============================================================================
            // 解释分析类节点（共 12 个，⚠ 属性参数均为行业模板，暂为虚构）
            // ============================================================================

            // 解释分析 - 产出剖面解释（⚠ 参数暂为虚构）(funcId=25)
            NodeMeta makeProfileInterpMeta()
            {
                NodeMeta m;
                m.typeId = "interpret.profile_interp";
                m.displayName = QObject::tr("产出剖面解释");
                m.category = QObject::tr("解释分析");
                m.group = NodeGroup::Interpret;
                m.kind = NodeKind::InOut;
                m.funcId = 25;
                m.funcName = "func_profile_interp";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("解释结果"), DataFormat::HDF5}};
                m.description = QObject::tr("产出剖面解释分析。");
                m.order = 100;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 解释分析 - 注入剖面解释（⚠ 参数暂为虚构）(funcId=26)
            NodeMeta makeInjectInterpMeta()
            {
                NodeMeta m;
                m.typeId = "interpret.inject_interp";
                m.displayName = QObject::tr("注入剖面解释");
                m.category = QObject::tr("解释分析");
                m.group = NodeGroup::Interpret;
                m.kind = NodeKind::InOut;
                m.funcId = 26;
                m.funcName = "func_inject_interp";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("解释结果"), DataFormat::HDF5}};
                m.description = QObject::tr("注入剖面解释分析。");
                m.order = 101;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 解释分析 - 多相流诊断（⚠ 参数暂为虚构）(funcId=27)
            NodeMeta makeMultiphaseDiagMeta()
            {
                NodeMeta m;
                m.typeId = "interpret.multiphase_diag";
                m.displayName = QObject::tr("多相流诊断");
                m.category = QObject::tr("解释分析");
                m.group = NodeGroup::Interpret;
                m.kind = NodeKind::InOut;
                m.funcId = 27;
                m.funcName = "func_multiphase_diag";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("诊断结果"), DataFormat::HDF5}};
                m.description = QObject::tr("多相流诊断分析。");
                m.order = 102;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 解释分析 - 压裂面解释分析（⚠ 参数暂为虚构）(funcId=28)
            NodeMeta makeFracInterpMeta()
            {
                NodeMeta m;
                m.typeId = "interpret.frac_interp";
                m.displayName = QObject::tr("压裂面解释分析");
                m.category = QObject::tr("解释分析");
                m.group = NodeGroup::Interpret;
                m.kind = NodeKind::InOut;
                m.funcId = 28;
                m.funcName = "func_frac_interp";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("解释结果"), DataFormat::HDF5}};
                m.description = QObject::tr("压裂面解释分析。");
                m.order = 103;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 解释分析 - 井间窜通解释分析（⚠ 参数暂为虚构）(funcId=29)
            NodeMeta makeInterwellInterpMeta()
            {
                NodeMeta m;
                m.typeId = "interpret.interwell_interp";
                m.displayName = QObject::tr("井间窜通解释分析");
                m.category = QObject::tr("解释分析");
                m.group = NodeGroup::Interpret;
                m.kind = NodeKind::InOut;
                m.funcId = 29;
                m.funcName = "func_interwell_interp";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("解释结果"), DataFormat::HDF5}};
                m.description = QObject::tr("井间窜通解释分析。");
                m.order = 104;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 解释分析 - 不稳定温度井解释（⚠ 参数暂为虚构）(funcId=30)
            NodeMeta makeUnstableInterpMeta()
            {
                NodeMeta m;
                m.typeId = "interpret.unstable_interp";
                m.displayName = QObject::tr("不稳定温度井解释");
                m.category = QObject::tr("解释分析");
                m.group = NodeGroup::Interpret;
                m.kind = NodeKind::InOut;
                m.funcId = 30;
                m.funcName = "func_unstable_interp";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("解释结果"), DataFormat::HDF5}};
                m.description = QObject::tr("不稳定温度井解释分析。");
                m.order = 105;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 解释分析 - 微地震解释分析（⚠ 参数暂为虚构）(funcId=31)
            NodeMeta makeMicroseismicInterpMeta()
            {
                NodeMeta m;
                m.typeId = "interpret.microseismic_interp";
                m.displayName = QObject::tr("微地震解释分析");
                m.category = QObject::tr("解释分析");
                m.group = NodeGroup::Interpret;
                m.kind = NodeKind::InOut;
                m.funcId = 31;
                m.funcName = "func_microseismic_interp";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("解释结果"), DataFormat::HDF5}};
                m.description = QObject::tr("微地震解释分析。");
                m.order = 106;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 解释分析 - 井筒完整性解释（⚠ 参数暂为虚构）(funcId=32)
            NodeMeta makeWellboreInterpMeta()
            {
                NodeMeta m;
                m.typeId = "interpret.wellbore_interp";
                m.displayName = QObject::tr("井筒完整性解释");
                m.category = QObject::tr("解释分析");
                m.group = NodeGroup::Interpret;
                m.kind = NodeKind::InOut;
                m.funcId = 32;
                m.funcName = "func_wellbore_interp";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("解释结果"), DataFormat::HDF5}};
                m.description = QObject::tr("井筒完整性解释分析。");
                m.order = 107;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 解释分析 - 井眼轨迹计算（⚠ 参数暂为虚构）(funcId=33)
            NodeMeta makeWelltrackCalcMeta()
            {
                NodeMeta m;
                m.typeId = "interpret.welltrack_calc";
                m.displayName = QObject::tr("井眼轨迹计算");
                m.category = QObject::tr("解释分析");
                m.group = NodeGroup::Interpret;
                m.kind = NodeKind::InOut;
                m.funcId = 33;
                m.funcName = "func_welltrack_calc";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("计算结果"), DataFormat::HDF5}};
                m.description = QObject::tr("井眼轨迹计算。");
                m.order = 108;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 解释分析 - 流压曲线计算（⚠ 参数暂为虚构）(funcId=34)
            NodeMeta makeFlowpresCalcMeta()
            {
                NodeMeta m;
                m.typeId = "interpret.flowpres_calc";
                m.displayName = QObject::tr("流压曲线计算");
                m.category = QObject::tr("解释分析");
                m.group = NodeGroup::Interpret;
                m.kind = NodeKind::InOut;
                m.funcId = 34;
                m.funcName = "func_flowpres_calc";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("计算结果"), DataFormat::HDF5}};
                m.description = QObject::tr("流压曲线计算。");
                m.order = 109;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 解释分析 - 微差井温计算（⚠ 参数暂为虚构）(funcId=35)
            NodeMeta makeDiffTempCalcMeta()
            {
                NodeMeta m;
                m.typeId = "interpret.diff_temp_calc";
                m.displayName = QObject::tr("微差井温计算");
                m.category = QObject::tr("解释分析");
                m.group = NodeGroup::Interpret;
                m.kind = NodeKind::InOut;
                m.funcId = 35;
                m.funcName = "func_diff_temp_calc";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("计算结果"), DataFormat::HDF5}};
                m.description = QObject::tr("微差井温计算。");
                m.order = 110;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // 解释分析 - 分层产量计算（⚠ 参数暂为虚构）(funcId=36)
            NodeMeta makeZonalProdCalcMeta()
            {
                NodeMeta m;
                m.typeId = "interpret.zonal_prod_calc";
                m.displayName = QObject::tr("分层产量计算");
                m.category = QObject::tr("解释分析");
                m.group = NodeGroup::Interpret;
                m.kind = NodeKind::InOut;
                m.funcId = 36;
                m.funcName = "func_zonal_prod_calc";
                m.inputs = {{"in", QObject::tr("输入数据"), DataFormat::HDF5}};
                m.outputs = {{"out", QObject::tr("计算结果"), DataFormat::HDF5}};
                m.description = QObject::tr("分层产量计算。");
                m.order = 111;
                m.externalProcess = true;
                return finalizeMeta(m);
            }

            // ============================================================================
            // 展示类节点（共 5 个，纯输入；⚠ 属性参数均为行业模板，暂为虚构）
            // ============================================================================

            // 展示 - 解释结果展示（⚠ 参数暂为虚构）(funcId=37)
            NodeMeta makeInterpResultDisplayMeta()
            {
                NodeMeta m;
                m.typeId = "display.interp_result";
                m.displayName = QObject::tr("解释结果展示");
                m.category = QObject::tr("可视化");
                m.group = NodeGroup::Display;
                m.kind = NodeKind::PureInput;
                m.funcId = 37;
                m.funcName = "func_interp_result_display";
                m.inputs = {{"in", QObject::tr("解释结果"), DataFormat::HDF5}};
                m.outputs = {};
                m.description = QObject::tr("展示解释分析结果。");
                m.order = 200;
                m.externalProcess = false;
                return finalizeMeta(m);
            }

            // 展示 - 预处理结果展示（⚠ 参数暂为虚构）(funcId=38)
            NodeMeta makePreprocessResultDisplayMeta()
            {
                NodeMeta m;
                m.typeId = "display.preprocess_result";
                m.displayName = QObject::tr("预处理结果展示");
                m.category = QObject::tr("可视化");
                m.group = NodeGroup::Display;
                m.kind = NodeKind::PureInput;
                m.funcId = 38;
                m.funcName = "func_preprocess_result_display";
                m.inputs = {{"in", QObject::tr("预处理结果"), DataFormat::HDF5}};
                m.outputs = {};
                m.description = QObject::tr("展示预处理结果曲线。");
                m.order = 201;
                m.externalProcess = false;
                return finalizeMeta(m);
            }

            // 展示 - 多结果合并展示（⚠ 参数暂为虚构）(funcId=39)
            NodeMeta makeMultiResultDisplayMeta()
            {
                NodeMeta m;
                m.typeId = "display.multi_result";
                m.displayName = QObject::tr("多结果合并展示");
                m.category = QObject::tr("可视化");
                m.group = NodeGroup::Display;
                m.kind = NodeKind::PureInput;
                m.funcId = 39;
                m.funcName = "func_multi_result_display";
                m.inputs = {
                    {"in1", QObject::tr("结果1"), DataFormat::HDF5},
                    {"in2", QObject::tr("结果2"), DataFormat::HDF5},
                    {"in3", QObject::tr("结果3"), DataFormat::HDF5}};
                m.outputs = {};
                m.description = QObject::tr("合并展示多个分析结果。");
                m.order = 202;
                m.externalProcess = false;
                return finalizeMeta(m);
            }

            // 展示 - 井眼模型展示（⚠ 参数暂为虚构）(funcId=40)
            NodeMeta makeWellboreModelDisplayMeta()
            {
                NodeMeta m;
                m.typeId = "display.wellbore_model";
                m.displayName = QObject::tr("井眼模型展示");
                m.category = QObject::tr("可视化");
                m.group = NodeGroup::Display;
                m.kind = NodeKind::PureInput;
                m.funcId = 40;
                m.funcName = "func_wellbore_model_display";
                m.inputs = {{"in", QObject::tr("模型数据"), DataFormat::HDF5}};
                m.outputs = {};
                m.description = QObject::tr("井眼模型3D展示。");
                m.order = 203;
                m.externalProcess = false;
                return finalizeMeta(m);
            }

            // 展示 - 井筒结构建立与展示（⚠ 参数暂为虚构）(funcId=41)
            NodeMeta makeWellStructDisplayMeta()
            {
                NodeMeta m;
                m.typeId = "display.well_struct";
                m.displayName = QObject::tr("井筒结构建立与展示");
                m.category = QObject::tr("可视化");
                m.group = NodeGroup::Display;
                m.kind = NodeKind::PureInput;
                m.funcId = 41;
                m.funcName = "func_well_struct_display";
                m.inputs = {{"in", QObject::tr("结构数据"), DataFormat::HDF5}};
                m.outputs = {};
                m.description = QObject::tr("井筒结构建立与展示。");
                m.order = 204;
                m.externalProcess = false;
                return finalizeMeta(m);
            }

            void registerExternalNode(NodeRegistry &reg, NodeMeta (*maker)())
            {
                // maker() 内部已 finalizeMeta，勿重复调用以免 paramOrder 重复
                const NodeMeta meta = maker();
                reg.registerType(meta, [meta]()
                                 { return std::make_shared<ExternalProcessNode>(meta); });
            }

        } // anonymous namespace

        // ============================================================================
        // 注册所有真实算法节点
        // ============================================================================
        void registerProductionNodes()
        {
            auto &reg = NodeRegistry::instance();

            // 避免重复注册
            if (reg.hasType("preprocess.depth_correct"))
                return; // 已注册

            // ---------- 数据管理 ----------
            registerExternalNode(reg, makeDataInputMeta);

            // ---------- 预处理小组件 ----------
            registerExternalNode(reg, makeFormatConvertMeta);

            registerExternalNode(reg, makeDepthCorrectMeta);
            registerExternalNode(reg, makeDepthTimeCorrectMeta);
            registerExternalNode(reg, makeDataCropMeta);
            registerExternalNode(reg, makeDataMergeMeta);
            registerExternalNode(reg, makeDasConvertMeta);
            registerExternalNode(reg, makeLfdasExtractMeta);
            registerExternalNode(reg, makeFkAnalysisMeta);
            registerExternalNode(reg, makeFftExtractMeta);
            registerExternalNode(reg, makeBandpassFilterMeta);
            registerExternalNode(reg, makeDownsampleMeta);
            registerExternalNode(reg, makeFtsExtractMeta);
            registerExternalNode(reg, makeMainfreqSplitMeta);
            registerExternalNode(reg, makeFbeExtractMeta);
            registerExternalNode(reg, makeMovingAvgMeta);
            registerExternalNode(reg, makeSlowStrainMeta);
            registerExternalNode(reg, makeDtsDenoiseMeta);
            registerExternalNode(reg, makeTempCorrectMeta);
            registerExternalNode(reg, makeTempDiffMeta);
            registerExternalNode(reg, makeDtgsGradientMeta);
            registerExternalNode(reg, makeBaselineBuildMeta);
            registerExternalNode(reg, makeStrainDisturbMeta);
            registerExternalNode(reg, makeTempCouplingMeta);
            registerExternalNode(reg, makeBaselineCalibMeta);
            registerExternalNode(reg, makePhysConvertMeta);

            // ---------- 解释分析类 ----------
            registerExternalNode(reg, makeProfileInterpMeta);
            registerExternalNode(reg, makeInjectInterpMeta);
            registerExternalNode(reg, makeMultiphaseDiagMeta);
            registerExternalNode(reg, makeFracInterpMeta);
            registerExternalNode(reg, makeInterwellInterpMeta);
            registerExternalNode(reg, makeUnstableInterpMeta);
            registerExternalNode(reg, makeMicroseismicInterpMeta);
            registerExternalNode(reg, makeWellboreInterpMeta);
            registerExternalNode(reg, makeWelltrackCalcMeta);
            registerExternalNode(reg, makeFlowpresCalcMeta);
            registerExternalNode(reg, makeDiffTempCalcMeta);
            registerExternalNode(reg, makeZonalProdCalcMeta);

            // ---------- 展示类（externalProcess=false，仅占位元数据）----------
            registerExternalNode(reg, makeInterpResultDisplayMeta);
            registerExternalNode(reg, makePreprocessResultDisplayMeta);
            registerExternalNode(reg, makeMultiResultDisplayMeta);
            registerExternalNode(reg, makeWellboreModelDisplayMeta);
            registerExternalNode(reg, makeWellStructDisplayMeta);
        }

    } // namespace production
} // namespace processing