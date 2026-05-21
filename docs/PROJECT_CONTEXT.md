# oilPro 项目上下文

> 维护者：工程师 ly + AI
> 最近更新：2026-05-18（v1.0）

**项目定位**：C++/Qt6 桌面客户端，油井测井数据预处理工作流编辑器。

**构建状态**：✅ EXITCODE=0

**高 DPI**：`main.cpp` → `setupHiDpiBeforeQApplication()`；界面用布局器 + 图表 `Expanding`；字体用 `pointSize`（详见 `src/gui/workspace/processing/workflow/node_dialogs.README.md` §高 DPI）。

---

## 快速开始

```powershell
# 构建
cmd /c tools\build\build_oilpro.cmd

# 运行
D:\pro\Demo\oilPro\bin\windows\x64\DEBUG\StratumExplorer.exe

# 发版推送到 GitHub（commit 后必做）
git push origin master
```

远程仓库：https://github.com/liyue-2021/StratumExplorer  

每次 push 前更新 `docs/HANDOFF_CHECKLIST.md`、`docs/PROJECT_CONTEXT.md`、`docs/MIGRATION_GUIDE.md`（详见 HANDOFF §0）。

---

## 核心文件

| 文件 | 说明 |
|------|------|
| `docs/HANDOFF_CHECKLIST.md` | 进度记录、回归测试、待办 |
| `docs/BACKEND_HANDOFF.md` | 与后端 EXE 对接（JSON 任务配置；业务 HDF5 归后端） |
| `docs/BACKEND_TODOS.md` | 后端同事待办索引（`TODO(后端)` 汇总） |
| `docs/MIGRATION_GUIDE.md` | 换电脑时的环境配置 |
| `docs/node_specs.json` | 人工查阅用（**运行时不加载**，见 `docs/node_specs.README.md`） |
| `node_client_params.json` | 属性面板参数（运行时加载） |
| `src/core/service/processing/ProductionNodes.cpp` | 42 节点注册 |

---

## 架构

```
src/
  core/
    interfaces/         # 纯接口
      models/processing/   WorkflowTypes, NodeDefinition
      service/processing/ IWorkflowEngine, INodeFactory, IProcessNode ← STATIC库+moc_anchor
    service/processing/   # 实现
      NodeRegistry, WorkflowEngine, ProductionNodes(42节点), ExternalProcessRunner(JSON配置)
      LogBus, DemoNodes(占位)
  gui/workspace/processing/workflow/  # UI
      WorkflowEditorTab, WorkflowScene, WorkflowView
      NodePalette(42节点), NodeItem, EdgeItem, PlotDialog, LogDock
```

---

## 关键约束

1. **UI 只走接口**：GUI 不直接 include `core/service/`
2. **Q_OBJECT 在 STATIC 库**：必须配 `moc_anchor.cpp`，否则 LNK2005
3. **任务配置协议**：前端写 `input_config_<id>.json` 调 EXE；业务 HDF5 由后端读写
4. **节点扩展**：注册 `NodeRegistry::instance().registerType(meta, creator)`

---

## 节点清单（42个）

| 分组 | 数量 | 示例 |
|------|------|------|
| Preprocess | 25 | 深度矫正、带通滤波、数据合并 |
| Interpret | 12 | 产出剖面解释、注入剖面解释 |
| Display | 5 | 预处理结果展示、多结果合并 |

---

## 常见问题

| 现象 | 解决 |
|------|------|
| `type_traits` 找不到 | 先 `vcvars64.bat` |
| LNK2005 | STATIC库 + moc_anchor.cpp |
| 中文乱码 | GBK控制台显示，**不是bug** |

---

## 传给新 AI 的话

```
你接手一个 Qt 6.10.3 + MSVC2022 的 C++ 桌面项目 oilPro。
项目路径：D:\pro\Demo\oilPro\

请按这个顺序读文档：
1. docs/PROJECT_CONTEXT.md  — 整体架构、约定、节点清单
2. docs/HANDOFF_CHECKLIST.md  — 当前进度、待办任务
3. docs/BACKEND_HANDOFF.md  — 与后端 EXE 的 JSON 配置协议

快速构建：
  cmd /c tools\build\build_oilpro.cmd

最近一次构建：EXITCODE=0（2026-05-16）
已注册 42 个真实算法节点（Preprocess 25个、Interpret 12个、Display 5个）
当前协议：JSON 任务配置文件调 EXE（业务 HDF5 由后端读写；非旧 stdin/stdout）
```

---

文档结束。有问题先看本文件，再看代码注释。
