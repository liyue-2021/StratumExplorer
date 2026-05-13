# oilPro 项目设计与开发上下文文档

> 本文档用于在不同电脑 / 不同 AI 助手之间传递项目背景，让协作可以无缝继续。
> 维护者：你 + GitHub Copilot
> 最近更新：Phase D 框架完成（保存路径记忆 / 跨进程调用器 / 全局日志 / 通用结果面板 / QCustomPlot），整体构建 EXITCODE=0。

---

## 1. 项目定位

`oilPro` 是一个 **C++ / Qt 6 桌面客户端**，目标是“油井测井数据的预处理、解释、展示”。
最先实现的子模块是 **预处理工作流编辑器**（PDF 需求 §3.1 / §3.2 / §3.3）：

- 用户在画布上拖拽节点，连成一个 DAG
- 每个节点完成一种数据处理（输入文件、滤波、相关分析、绘图……）
- 支持「单节点跑」「全部跑」「停止」三种运行方式
- 支持保存为「模板（不含数据 .wftpl）」「工程（含数据 .wfproj）」

---

## 2. 开发环境（必须）

| 工具       | 版本                     | 路径                                                                  |
|------------|--------------------------|-----------------------------------------------------------------------|
| Qt         | **6.10.3 MSVC2022 64bit** | `D:\Qt\6.10.3\msvc2022_64`                                            |
| CMake      | **3.30.5**（Qt 自带）    | `D:\Qt\Tools\CMake_64\bin\cmake.exe` ❗ **不要用系统的 4.0.0**         |
| 编译器     | MSVC 14.44.35207         | `C:\Program Files\Microsoft Visual Studio\2022\Community`             |
| jom        | Qt 自带                  | 由 Qt Creator / vcvars 环境拉起                                       |
| IDE        | Qt Creator 优先 / VS Code 备选 |                                                                 |
| 命令行编译 | 必须先 `vcvars64.bat`    | `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat` |

❗ 直接在 PowerShell 跑 `cmake --build` 会出现 `fatal error C1083: Cannot open include file: 'type_traits'`。  
**正确做法**（PowerShell）：

```powershell
$vc = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
$bd = "c:\Users\liyli2\Downloads\oil\oilPro\build\Desktop_Qt_6_10_3_MSVC2022_64bit-Debug"
cmd /c "`"$vc`" && `"D:\Qt\Tools\CMake_64\bin\cmake.exe`" --build `"$bd`""
```

> 或者直接在 Qt Creator 里 `构建`。

---

## 3. 顶层目录速览

```
oilPro/
  CMakeLists.txt                  # 根（已加 CMP0177 + CMAKE_POLICY_VERSION_MINIMUM 3.5 兼容 HDF5）
  cmake/Modules/                  # FindXxx + 自定义 module
  platform/                       # 各平台特定的资源/打包
  resources/                      # qrc/icons/styles
  translations/                   # i18n .ts → .qm（src/app/CMakeLists.txt 里 add_dependencies(app generate_qm)）
  src/
    app/                          # 入口 main.cpp + Application
    core/                         # 业务逻辑（不依赖 UI）
      interfaces/                 # 纯接口 + DTO（其他层只依赖这里）
        models/processing/        # WorkflowTypes / NodeDefinition
        service/processing/       # IWorkflowEngine / INodeFactory / IProcessNode  ← STATIC 库
      service/                    # 接口的实现
        processing/               # WorkflowEngine / NodeRegistry
    gui/                          # 所有 Qt 界面
      workspace/processing/       # 「数据处理」Tab 页 ProcessingPage
        workflow/                 # 工作流编辑器 (本次新增子模块)
  test/
  third_party/
```

---

## 4. 工作流子模块（本次重点）

### 4.1 已完成功能

| 类别     | 文件                                                    | 状态 |
|----------|---------------------------------------------------------|------|
| 数据类型 | `core/interfaces/models/processing/WorkflowTypes.h`     | ✅   |
|          | `core/interfaces/models/processing/NodeDefinition.h`    | ✅   |
| 接口     | `core/interfaces/service/processing/IProcessNode.h`     | ✅   |
|          | `core/interfaces/service/processing/INodeFactory.h`     | ✅   |
|          | `core/interfaces/service/processing/IWorkflowEngine.h`  | ✅ Q_OBJECT，添加了 `setNodePosition/setNodeSize` |
|          | `core/interfaces/service/processing/moc_anchor.cpp`     | ✅ MOC 锚点（必需）|
| 引擎实现 | `core/service/processing/NodeRegistry.h/cpp`            | ✅ 单例工厂 |
|          | `core/service/processing/WorkflowEngine.h/cpp`          | ✅ DAG + 调度 + 校验 + 序列化（包含 w/h）|
|          | `core/service/processing/DemoNodes.h/cpp`               | ✅ 占位 demo 节点实现 |
|          | `core/service/processing/ExternalProcessRunner.h/cpp`   | ✅ 跨进程 JSON 协议调用器（Phase D）|
|          | `core/service/processing/LogBus.h/cpp`                  | ✅ 全局日志总线（QObject 单例 + 按天落盘）|
| UI       | `gui/workspace/processing/workflow/WorkflowEditorTab.*` | ✅ 工具栏 / 三栏 / 底部状态栏 / 底部日志面板 |
|          | `gui/workspace/processing/workflow/WorkflowScene.*`     | ✅ 监听引擎信号 + 双击事件转发 |
|          | `gui/workspace/processing/workflow/WorkflowView.*`      | ✅ 滚轮缩放 / Ctrl+0 复位 / Ctrl+F 适应 |
|          | `gui/workspace/processing/workflow/NodePalette.*`       | ✅ 按 group 分类节点库 |
|          | `gui/workspace/processing/workflow/NodeItem.*`          | ✅ 自适应宽度 / 配色 / 状态文字 / ▶■ 按钮 / 端口 / 拖拽回写尺寸 |
|          | `gui/workspace/processing/workflow/EdgeItem.*`          | ✅ 贝塞尔 / 正交折线可切换，端点选择带对齐加权 |
|          | `gui/workspace/processing/workflow/PlotDialog.*`        | ✅ 双击节点弹出，基于 QCustomPlot，带 PNG/CSV 导出、顶部元信息区 |
|          | `gui/workspace/processing/workflow/LogDock.*`           | ✅ 底部日志面板，订阅 LogBus ＋ 等级/关键字过滤 |
| 三方依赖 | `third_party/qcustomplot/`                              | ✅ QCustomPlot 2.1.1 以 STATIC 库集成。**不要**定义 `QCUSTOMPLOT_USE_LIBRARY`，否则会 Q_DECL_IMPORT 报 LNK |
| 集成     | `gui/workspace/processing/ProcessingPage.cpp`           | ✅ 创建 WorkflowEngine + 内嵌 WorkflowEditorTab |

### 4.2 关键设计约束

1. **UI / 业务严格分离**：UI 只通过 `IWorkflowEngine`、`INodeFactory` 接口与 core 通信。新加 UI 不允许直接 include `core/service/`。
2. **单一数据源**：UI 操作（拖拽、连线、删节点）→ 调引擎 → 引擎成功后 emit 信号 → UI 重绘。UI 不持有重复状态。
3. **Q_OBJECT 必须在 STATIC 库里**：`core_interfaces_service_processing` 是 STATIC 而不是 INTERFACE，否则每个消费者都会重新 MOC 一份导致 `LNK2005`。`moc_anchor.cpp` 是为了让 STATIC 库有真的 .obj 让 MOC 产物链上去。
4. **HDF5 暂时关闭**：`-DWITH_HDF5=OFF`。所有 `target_link_libraries(... hdf5)` 都包了 `if(WITH_HDF5)`。
5. **节点类型可扩展**：新节点 = 实现 `IProcessNode` + `NodeRegistry::instance().registerType(meta, []{ return std::make_unique<MyNode>(); });`，UI 完全不用改。

### 4.3 视觉规范（已实现）

| 元素          | 设计                                                         |
|---------------|--------------------------------------------------------------|
| 节点宽度      | 根据标题/类型/状态自适应，160 ~ 320 px，超长省略号 + Hover 完整提示 |
| 节点底色      | DataInput=蓝, Preprocess=橙, Interpret=紫, Display=绿        |
| 顶部色带      | groupColor 加深 15%                                          |
| 状态显示      | 文字 + 圆点：Idle 灰 / Running 橙 / Succeeded 绿 / Failed 红 / Stopped 深灰 / Error 品红 |
| 启动 / 停止   | ▶ (绿色三角) / ■ (红色方块)，鼠标点击命中按钮                |
| 端口          | 左中=输入(黄圆)，右中=输出(绿圆)；PureInput/PureOutput 隐藏对应端口 |
| 连线          | Manhattan 直角折线，蓝色 2px，终点带箭头                     |
| 校验失败      | 底部状态栏红字提示 5 秒后自动恢复                            |
| 不在 UI 上展示日志 | 只在 status bar 显示当前事件，详细日志走 `qInfo()` 后台    |

### 4.4 文件格式

- `.wftpl` 模板：`engine.serialize(false)` —— 仅拓扑、参数
- `.wfproj` 工程：`engine.serialize(true)` —— 拓扑 + 参数 + 输入/输出文件路径

---

## 5. 工作中常见问题与解决

| 现象                                                                   | 原因 & 解决                                                                |
|------------------------------------------------------------------------|----------------------------------------------------------------------------|
| `fatal error C1083: 'type_traits'`                                     | 没在 MSVC 环境跑。先 `call vcvars64.bat`，或用 Qt Creator                 |
| `LNK2005 IWorkflowEngine::xxx 已经在 ... 中定义`                       | Q_OBJECT 在 INTERFACE 库里被多次 MOC。改成 STATIC + moc_anchor.cpp         |
| `HDF5` FetchContent 失败                                               | `-DWITH_HDF5=OFF`，等以后用 vcpkg                                          |
| 控制台打印 `?????`                                                     | 中文 UTF-8 在 GBK 控制台显示，**不是 bug**                                 |
| Qt Creator 卡在 `9/34`                                                 | Defender 实时扫描 build 目录，把 `oilPro\build` 加到排除项                 |
| 翻译 .qm 找不到                                                        | `add_dependencies(app generate_qm)` 已加在 `src/app/CMakeLists.txt`        |
| 窗口看起来空白                                                         | 默认尺寸太小，按 `Win+↑` 最大化即可                                        |
| `cmake.exe : no such file or directory` + `jom: ... Error 2`           | jom 不在 PATH，且不在 vcvars 里跑。用 vcvars64.bat 包一层                  |

---

## 6. 待办（按优先级）

### P0 — 跟后端对接后立即能做

- [ ] 把 `DemoNodes` 逐个替换为“调后端 EXE”的真实节点：
      节点 `run()` 里 `ExternalProcessRunner::runBlocking(req, onProgress)` 代替
      现在的 sleep 占位实现。`req.exePath` 暂从节点参数 `exePath` 取，
      后续会接入统一的 ToolRegistry / 全局设置。
- [ ] `PlotDialog::setSeries()` 已就位，接入后只需把 `setDemoFromSeed` 调用
      换成解析后端输出 HDF5 后的 `setSeries(...)`。

### P1 — UX 完善

- [ ] 鼠标拖拽连线：从输出端口拖出 → 释放在输入端口
  - 在 `WorkflowScene::mousePressEvent` 检测端口附近，画临时折线，release 调 `engine->addEdge(...)`
- [ ] 节点右键菜单：重命名 / 删除 / 单跑 / 编辑参数
- [ ] 选中节点 → 右侧属性面板根据 `NodeMeta::defaultParams` 动态生成表单
- [ ] 拖拽 NodePalette 项目到画布（DnD 替代/补充双击）
- [ ] LogDock 加“导出日志到文件”按钮（现在每天自动落盘，不能手动另存）

### P2 — 业务功能

- [ ] 多结果合并节点（kind=PureInput，可接多条上游输出）
- [ ] 接入 SQLite DAO 做项目/模板的持久化（`core/dao/project/`）
- [ ] 启用 HDF5（vcpkg manifest 模式）
- [ ] 多 Tab 工程编辑（现在是单 Tab）

### P3 — 工程化

- [ ] 单元测试：WorkflowEngine 的 DAG 校验、序列化、调度
- [ ] CI：GitHub Actions Windows-latest + Qt action
- [ ] i18n：把所有 `tr("...")` 字符串补到 `translations/global_zh_cn.ts`

---

## 7. 给下一位 AI 助手的接力提示词

> 把下面这段话粘贴给新 AI 即可恢复上下文。

```text
你接手一个 Qt 6.10.3 + MSVC2022 的 C++ 桌面项目 oilPro，路径 D:\pro\oilPro。
请先读 PROJECT_CONTEXT.md（在项目根）和 BACKEND_HANDOFF.md（后端接口汇总）。

项目现状：
- 桌面端在做「预处理工作流编辑器」。
- 架构：core/interfaces (纯接口) + core/service (实现) + gui/* (UI)；UI 只能走接口。
- IWorkflowEngine 必须放在 STATIC 库里 + 配 moc_anchor.cpp，否则多个 cpp MOC 会冲突。
- HDF5 已禁用（WITH_HDF5=OFF）。
- 命令行编译必须先 vcvars64.bat；否则会缺 type_traits。
- QCustomPlot 2.1.1 在 third_party/qcustomplot/，**不要**定义 QCUSTOMPLOT_USE_LIBRARY。
- 最近一次构建 EXITCODE=0。

Phase D 已完成的框架：
1. 保存路径记忆（m_currentFilePath/m_currentIsTemplate）+ “另存为”按钮
2. ExternalProcessRunner：跨进程 JSON 协议的算法调用器（单任务单进程）
3. LogBus（QObject 单例 + %APPDATA%/StratumExplorer/logs/run_YYYYMMDD.log 落盘）+ LogDock
4. PlotDialog 通用结果面板（display.* 节点画曲线，其他节点显示输出文件信息）

待办 P0：接入后端算法。后端的事项看 BACKEND_HANDOFF.md。
```

---

## 8. 一些约定

- **UTF-8 BOM**：所有源文件统一 UTF-8（无 BOM 也行，MSVC 用 `/utf-8` 编译选项）
- **命名空间**：core 用 `processing::`，UI 用 `processing::gui::`
- **头文件保护**：用 `#ifndef XXX_H` 而不是 `#pragma once`（已有约定）
- **Qt 信号**：永远用 `connect(sender, &Class::signal, receiver, &Class::slot)` 函数指针风格
- **不要把 UI 头文件 include 到 core**：`#include <Q...Widget>` 必须出现在 gui/ 目录里

---

文档结束。任何疑问优先看本文件，再看代码注释，最后问人。
