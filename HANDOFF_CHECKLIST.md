# HANDOFF_CHECKLIST.md · 项目最终交接清单

> 作者：工程师 ly
> 用途：项目暂时离手时给接手人 / 后端 / 自己半年后回来看的"一页纸"清单。
> 关联文档：[BACKEND_HANDOFF.md](./BACKEND_HANDOFF.md) · [MIGRATION_GUIDE.md](./MIGRATION_GUIDE.md) · [DESIGN_DRAFT.md](./DESIGN_DRAFT.md) · [PROJECT_SHOWCASE.md](./PROJECT_SHOWCASE.md)

---

## 0. 项目现状速查

| 项 | 状态 |
|---|---|
| 完整源码 | `c:\Users\liyli2\Downloads\oil\oilPro\` （此目录） |
| 演示精简版 | `c:\Users\liyli2\Downloads\oil\oilPro_demo\` |
| 完整版打包 zip | `c:\Users\liyli2\Downloads\oil\oilPro_handoff.zip`（512KB，无 build/） |
| 构建脚本 | `C:\Temp\build_oilpro.cmd` / `C:\Temp\build_oilpro_demo.cmd` |
| 编译产物 | `oilPro\bin\windows\x64\DEBUG\StratumExplorer.exe` |
| Build 状态 | EXITCODE=0（最近一次：QSS 样式接入后） |

---

## 1. 后端要接的全部代码点（grep "NEED BACKEND TO DO" 即可定位）

| 文件 | 在做什么 | 后端要做什么 |
|---|---|---|
| [INodeFactory.h](src/core/interfaces/service/processing/INodeFactory.h) | 节点工厂接口 | 新建 RealNodes.cpp 注册真实算法节点 |
| [DemoNodes.cpp](src/core/service/processing/DemoNodes.cpp) | 全部是占位节点 | 把每条 `registerType` 的 lambda 换成真实节点 |
| [ExternalProcessNode.cpp](src/core/service/processing/ExternalProcessNode.cpp) | 子进程节点的统一入口 | 按 BACKEND_HANDOFF.md §3 的 JSON 协议写算法 EXE/PY |
| [WorkflowEditorTab.cpp](src/gui/workspace/processing/workflow/WorkflowEditorTab.cpp) `onNodeDoubleClicked` | display.* 节点弹图 | 把 `setDemoFromSeed` 替换成读 HDF5 → `setSeries(...)` |
| [DevicePage.cpp](src/gui/workspace/device/DevicePage.cpp) | 设备控制页 UI | 实现 IP/Port 测连、保存到 SQLite、采集开关接入 |

> 检索命令：`grep -rn "NEED BACKEND TO DO" src/`

---

## 2. 已知 P0 / P1 问题（接手第一天可能踩到）

### P0 必须修
- 无（最近一次 build EXITCODE=0，可直接运行）。

### P1 建议尽快修
1. **WorkflowEditorTab 头文件里 `m_logDock` 在 demo 版未声明清理** — demo 版我没删 `m_logDock` 成员声明，只是不 new 它，留着 nullptr 不影响功能但有点丑。
2. **CoreContext 是单例 + Service Locator** — 测试不友好。如果后续要补单元测试，建议改成构造注入。
3. **HDF5 读取走的是 DAO 同步接口** — 大文件会卡 UI 线程，建议加一层 `QtConcurrent::run` 包装。
4. **节点参数面板 `rebuildParamForm` 每次都重建 widget** — 切换节点时有视觉闪烁，可改增量更新。
5. **国际化 `.ts` 没扫全**：新加的 NEED BACKEND 注释、设备页文本都需要重跑 `lupdate`。

### P2 可选优化（在 PROJECT_SHOWCASE.md §4.2 里有详细的 8 条）
- HDF5 hyperslab 按视图按需读
- 节点结果哈希缓存
- 子进程 worker pool 复用
- QGraphicsView LOD（缩放小不画端口文字）
- DAG 关键路径调度 + 算子融合
- QCustomPlot 增量重绘
- SQLite WAL 异步写
- GoogleTest + Qt Test 补测试

---

## 3. UI 风格规范（截图驱动，已经全局生效）

QSS 在 [resources/styles/light.qss](resources/styles/light.qss)，启动时由 [ThemeManager](src/gui/utils/ThemeManager.cpp) 自动加载。

新写控件时只要给 `objectName` 或 `dynamic property` 就能直接套样式：

| 想要效果 | 怎么标 |
|---|---|
| 蓝色主按钮 | `setProperty("role","primary")` |
| 浅黄色辅助按钮（如"数据管理"） | `role="warning"` |
| 红色危险按钮 | `role="danger"` |
| 纯文字图标按钮 | `role="ghost"` 或用 `QToolButton` |
| DAS / DTS / DSS 三色卡片 | objectName = `DasCard` / `DtsCard` / `DssCard` |
| 通用白卡片 | objectName = `Card` |
| 大标题 / 小标题 / 灰色提示 | QLabel + `role="title"`/`"subtitle"`/`"muted"` |
| 圆角搜索框 | QLineEdit + `role="search"` |

> 运行时改 property 后需 `style()->unpolish(w); style()->polish(w);` 才会刷新；设计器里设的不用。

> 性能注意：`QGraphicsView` 在 QSS 里只设了透明背景 + 一行边框，不要再往里加复杂规则，否则节点编辑器会卡。

---

## 4. 构建 & 跑起来（30 秒上手）

```powershell
# 完整版
cmd /c C:\Temp\build_oilpro.cmd
.\oilPro\bin\windows\x64\DEBUG\StratumExplorer.exe

# 精简演示版
cmd /c C:\Temp\build_oilpro_demo.cmd
.\oilPro_demo\bin\windows\x64\DEBUG\StratumExplorer.exe
```

环境要求：详见 [MIGRATION_GUIDE.md §1](MIGRATION_GUIDE.md)。最关键：
- Qt 6.10.3 MSVC2022 64bit
- CMake 3.21 ~ 3.30（**不要用 4.x**）
- jom + vcvars64.bat 必须在 PATH 里（脚本已自动加）

---

## 5. 本次离手前都改了什么（changelog）

1. ✅ Phase D 完成：`ExternalProcessRunner` / `ExternalProcessNode` / `LogBus` / `LogDock` / `PlotDialog` / QCustomPlot 集成
2. ✅ 4 个新文件加 "// 作者：工程师 ly" 头注释
3. ✅ 新增文档：[BACKEND_HANDOFF](BACKEND_HANDOFF.md) / [MIGRATION_GUIDE](MIGRATION_GUIDE.md) / [DESIGN_DRAFT](DESIGN_DRAFT.md) / [PROJECT_SHOWCASE](PROJECT_SHOWCASE.md)
4. ✅ 更新 [PROJECT_CONTEXT.md](PROJECT_CONTEXT.md) 反映 Phase D
5. ✅ 打包 oilPro_handoff.zip
6. ✅ 克隆出 oilPro_demo/ 精简演示版（无连线类型切换、无 LogDock、无双击弹图）
7. ✅ 全局 QSS 风格接入（参照截图蓝白扁平），ThemeManager 自动加载
8. ✅ Tab 高度从 10px padding 收窄到 4px，全局字号从 13px 降到 12px
9. ✅ QGraphicsView 排除出 QSS 复杂规则（解决卡顿、节点字过大）
10. ✅ 5 个对接点加 "NEED BACKEND TO DO" 显式注释

---

## 6. 一句话回归测试（每次合并代码后跑）

1. 双击 exe → 主窗口出现，4 个顶部 tab（井管理 / 实时数据 / 数据处理工作流 / 设备控制）字号正常、tab 不超高
2. 进入"数据处理工作流"，左侧节点库可以拖一个"LAS 数据输入"到画布
3. 拖第二个"带通滤波"，把上一个节点的 out 端口连到新节点的 in 端口
4. 点工具栏"运行" → 节点状态变绿，底部 LogDock 出现 sleep 假任务的进度
5. 双击"折线图"节点 → 弹出 PlotDialog 显示占位曲线
6. 关闭 → 重开，节点能恢复（说明 SQLite 持久化没坏）

---

*以上 6 步全过，说明项目处于可继续开发的健康状态。*
