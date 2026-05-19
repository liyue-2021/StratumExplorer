# HANDOFF_CHECKLIST.md · 项目进度记录

> 作者：工程师 ly + AI  
> 最近更新：2026-05-18（v1.0 工作流 + 属性面板大版本）

---

## 项目现状

| 项 | 状态 |
|---|---|
| 源码路径 | `D:\pro\Demo\oilPro\` |
| 构建脚本 | `tools\build\build_oilpro.cmd` |
| 编译产物 | `bin\windows\x64\DEBUG\StratumExplorer.exe` |
| 构建状态 | **EXITCODE=0** |
| 当前阶段 | **v1.0 已交付**：42 节点 + 中文属性面板 + 后端 JSON 配置协议 |
| 远程仓库 | https://github.com/liyue-2021/StratumExplorer |
| 最近 push | `41628de` → `origin/master`（2026-05-18） |

---

## 0. 发版与 Git 同步（每次必做）

本地 `git commit` **不会**自动上传到 GitHub，发版或阶段性交付后必须 **push**。

### 0.1 推荐命令顺序

```powershell
cd D:\pro\Demo\oilPro

# 1. 构建通过
cmd /c tools\build\build_oilpro.cmd

# 2. 本地提交
git status
git add <相关文件>
git commit -m "release(vX.Y): 简要说明"

# 3. 推送到 GitHub（必做）
git push origin master

# 4. 确认
git log -1 --oneline
# 浏览器打开仓库确认最新 commit
```

远程地址：`git@github.com:liyue-2021/StratumExplorer.git`（`origin`）

### 0.2 每次 push 前必须更新的记录文档

| 文件 | 更新内容 |
|------|----------|
| **`docs/HANDOFF_CHECKLIST.md`** | 本表「项目现状」、版本节、待办、**最近 push 的 commit/hash/日期** |
| **`docs/PROJECT_CONTEXT.md`** | 文首「最近更新」、协议/架构若有变 |
| **`docs/MIGRATION_GUIDE.md`** | §0 开场白里的版本、构建状态、下一版本方向 |
| `docs/BACKEND_HANDOFF.md` | 仅当后端对接协议、IO 字段、JSON 结构有变时 |
| `README.md` | 仅当依赖、目录说明、使用方式有变时 |

代码有功能变更但文档未同步 → **不要 push**。

### 0.3 发版检查清单

- [ ] `tools\build\build_oilpro.cmd` → EXITCODE=0  
- [ ] 已 `git commit`  
- [ ] 已更新上表中的记录文档  
- [ ] 已 `git push origin master`  
- [ ] GitHub 页面能看到最新 commit  

---

## 1. v1.0 大版本（2026-05-18）

### 1.1 新增

| 文件 | 说明 |
|------|------|
| `ProductionNodes.h/cpp` | 42 个节点注册（预处理 25 + 解释 12 + 展示 5） |
| `ProductionNodeParams.h/cpp` | 从 JSON 加载默认参数 / 中文标签 / 展示顺序 |
| `node_client_params.json` | **属性面板运行时真源**（23 真实 + 19 虚构占位） |
| `node_client_params.qrc` | 参数表嵌入 exe |
| `node_specs.README.md` | 说明 `node_specs.json` 仅人工查阅 |

### 1.2 修改

| 文件 | 说明 |
|------|------|
| `ExternalProcessRunner` | 任务 **JSON 配置文件** 调 EXE（`OILPRO_PROTOCOL=JSON`） |
| `ExternalProcessNode` | `funcId`/`funcName`、6 路 IO；展示节点不启 EXE |
| `WorkflowEditorTab` | 动态属性面板（中文、顺序、数组、旧预设补参） |
| `WorkflowEngine` | 新建/反序列化时合并 `node_client_params` 默认参数 |
| `DemoNodes` | 清空，由 ProductionNodes 接管 |
| `BACKEND_HANDOFF.md` | 后端对接文档（JSON 配置 + 业务 HDF5 归后端） |

### 1.3 参数数据约定

| 用途 | 文件 |
|------|------|
| 属性面板 | `src/core/service/processing/node_client_params.json` |
| 节点端口 / funcId | `ProductionNodes.cpp` |
| 人工对照（不加载） | `node_specs.json` |

---

## 2. 协议（当前）

### 前端 → 后端 EXE

```
exe.exe  <path>/input_config_<taskId>.json
```

- 配置内容为 JSON（字段语义见 `BACKEND_HANDOFF.md`）
- 业务数据文件仍为 **HDF5**，由后端 EXE 读写
- 结果：`<output>/task_<taskId>.json` 或 stdout 打印路径

### 已废弃

- stdin/stdout 一行 JSON 事件流（旧 Demo 协议）

---

## 3. 回归测试

1. 启动 exe → 主窗口正常
2. 进入「工作流编排」→ 节点库 42 个节点、三分组
3. 拖入「深度矫正」→ 右侧中文参数（插值方法、深度步长等）
4. 拖入「带通滤波」→ 连线成功
5. 拖入「预处理结果展示」→ 有参数面板、**运行不报 exePath 错误**
6. 保存/重开预设 → 参数仍完整
7. （可选）配置 exePath 后运行预处理节点 → 需后端提供 EXE

---

## 4. 下一版本：展示界面开发（v1.1+）

| 优先级 | 任务 | 说明 |
|--------|------|------|
| P0 | PlotDialog 接真实数据 | 读上游 HDF5 / `data_path` 参数 |
| P0 | 展示类节点 UI | 5 个 Display 节点的专用展示面板 |
| P1 | 虚构参数替换 | 甲方确认后更新 19 个 `fictional` 节点 |
| P1 | 属性面板：虚构节点提示 | `clientParamsFictional` 显示「参数待确认」 |
| P2 | 后端首个 EXE 联调 | 按 `BACKEND_HANDOFF.md` 对接 |

---

*以上回归通过即可进入展示界面开发。*
