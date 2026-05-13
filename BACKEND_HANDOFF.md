# oilPro 前后端对接备忘 — BACKEND_HANDOFF

> **场合**：和后端/算法同事对一次「前端是怎么跑算法的、需要后端给什么」。
> **作者**：工程师 ly
> **当前版本**：Phase D 框架已就绪（构建 EXITCODE=0）。

---

## 1. 一句话定位

桌面端只负责**画 DAG、调度、把任务交给算法 EXE、收集输出**。
**算法本身**（任何预处理 / 解释计算）由后端写成 EXE，前端通过**单任务一进程 + JSON 协议**调用。

```
   ┌─────────── 桌面端 (Qt/C++) ───────────┐
   │  WorkflowEditor (拖拽 DAG)             │
   │       │                                │
   │       ▼                                │
   │  WorkflowEngine (调度，并发，取消)      │
   │       │                                │
   │       ▼                                │
   │  IProcessNode::run()                   │   ← 节点实现里只调 Runner
   │       │                                │
   │       ▼                                │
   │  ExternalProcessRunner ──── stdin/stdout/stderr ────► 后端算法 EXE
   └────────────────────────────────────────┘
```

---

## 2. 后端要交付的最小集合

对**每一种**算法节点，后端要交：

| 项 | 说明 | 谁定 |
|---|---|---|
| ① 一个 EXE 文件 | 命令行可执行，符合下面的 JSON 协议 | 后端 |
| ② 节点元数据描述（5 个字段） | 我前端按这个注册到 NodeRegistry | 双方约 |
| ③ 输入输出文件格式 | 期望/产出的格式（HDF5 / LAS / CSV / …）| 双方约 |
| ④ 参数清单 | 名字、类型、默认值、合法范围 | 后端 |
| ⑤ 一份样例输入 + 期望输出 | 我用来联调 | 后端 |

**规模评估**：第一轮先约 3~5 个节点跑通整条链路（输入 + 1~2 个预处理 + 1 个展示），后续按表格批量补。

---

## 3. JSON 调用协议（最重要）

### 3.1 父进程 → 子进程（stdin，**一行**）

```json
{
  "op":     "run",
  "params": { "...节点参数..." },
  "inputs": { "in": ["C:/data/A.h5"], "in2": ["C:/data/B.h5"] },
  "output": "C:/temp/oilpro/<nodeId>.h5"
}
```

- `params`：节点参数（前端会把 UI 表单的值原样塞进来）
- `inputs`：按"端口键"组织的输入文件列表。键名由节点元数据里的 `PortSpec.key` 决定（例如 `in` / `seismic` / `well`）
- `output`：前端建议的输出路径。**算法可以忽略**这个路径，自报实际产出（见 done 事件）

### 3.2 子进程 → 父进程（stdout，**每行一条 JSON**）

```json
{"event":"progress","value":42,"message":"reading well A"}
{"event":"log","message":"normalize done"}
{"event":"done","outputs":["C:/temp/oilpro/<nodeId>.h5"]}
{"event":"error","message":"file not found: A.h5"}
```

- 进度 `value` ∈ [0, 100]
- `done` **必须**最后发一次（成功）；`outputs` 是真正落盘的产物路径数组
- `error` 发生后请立即 exit 非 0
- stderr：自由文本，前端会原样转发到日志面板（适合 `printf("debug: ...")`）

### 3.3 退出码

| exit | 含义 |
|---|---|
| 0 | 成功（必须配合 `done` 事件） |
| 非 0 | 失败 |

如果只 exit 0 但没发 done 事件，前端**容错**为成功（避免历史脚本不兼容）。

### 3.4 取消

前端点"停止"时：
1. `QProcess::terminate()`（Windows 下发 WM_CLOSE / Ctrl+Break）
2. 5 秒内未退出 → `kill()`

**后端要做的事**：在长循环里处理 SIGTERM / Ctrl 信号，能优雅退出最好（不强求）。

---

## 4. 前端对每个节点的"画像"（NodeMeta）

前端要在 `NodeRegistry` 里注册每个节点，最小信息：

```cpp
NodeMeta{
    .typeId        = "preprocess.bandpass",   // 全局唯一字符串 ID
    .displayName   = "带通滤波",              // 节点库 / 画布上显示的中文名
    .category      = "DAS 处理",              // 二级分类（节点库分组用）
    .group         = NodeGroup::Preprocess,    // 大组：DataInput/Preprocess/Interpret/Display
    .kind          = NodeKind::InOut,          // 数据流方向：PureOutput / InOut / PureInput
    .inputs        = { {"in",  "输入信号", DataFormat::HDF5} },
    .outputs       = { {"out", "滤波结果", DataFormat::HDF5} },
    .defaultParams = { {"lowHz", 5.0}, {"highHz", 200.0}, {"order", 4} },
    .description   = "对 DAS 信号做带通滤波。",
    .externalProcess = true                    // 走 stdin/JSON 调 EXE
};
```

**后端需要给我的**：上面这张表里每个字段的值（除 `kind` / `group` 我可以猜，其他都得后端告诉我）。

> 模板：见本文档末尾 **附录 A：节点登记单**。

---

## 5. 数据流约束（很重要，连线时前端会校验）

### 5.1 节点种类

| Kind | 含义 | 例子 |
|---|---|---|
| `PureOutput` | 没有上游，只产数据 | LAS 读取、HDF5 读取、合成数据 |
| `InOut` | 有上游 → 处理 → 产出 | 滤波、去噪、相关分析 |
| `PureInput` | 有上游，只看不再产 | 折线图、热图、报告 |

### 5.2 数据格式（DataFormat 枚举）

```
HDF5 / LAS / LTD / TDMS / SEGY / CSV / MAT / WITS / JSON / XML / WellboreModel / WellStructure
```

**项目内部统一 HDF5**（PDF §5.1.5）。
- 数据输入节点可以读 LAS/CSV/… 然后**输出 HDF5**；
- 中间预处理节点**输入输出都应该是 HDF5**；
- 展示节点输入 HDF5。

### 5.3 端口校验规则（前端自动做）

- 上游 `outputs[k].format` 必须等于下游 `inputs[k].format`，否则连线被拒绝
- 下游用 `Unknown` 表示"接收任意"
- 一个节点可有**多个**输入端口（多上游），如多结果合并节点

---

## 6. 输出文件规范

为了让"运行 → 双击节点查看"这条链路可视化能复用，请保证：

### 6.1 单一 HDF5 文件结构（建议）

```
/data
    ├── x          (N,)    或 (M, N)   横轴 / 时间
    ├── y          (N,)    或 (M, N)   主曲线
    └── series/    可选，多曲线场景
         ├── 0     (N,)
         ├── 1     (N,)
         └── ...
/meta
    ├── unit_x          string
    ├── unit_y          string
    ├── description     string
    └── ...
```

- `display.line` 节点：读 `/data/x` 和 `/data/y`
- `display.merge` 节点：读 `/data/series/*`

> **当前阶段**：前端 PlotDialog 里画的是占位演示数据；只要后端按上面格式产出 HDF5，我把 `setDemoFromSeed(...)` 换成 `setSeries(读 HDF5)` 就行。

### 6.2 命名

输出文件路径 **建议**用前端给的 `output` 字段；如果算法内部要拆成多个文件，就在 `done.outputs` 里把所有产物路径回报。

### 6.3 HDF5 内部结构 ‑ 谁定义、谁读取

| 工作 | 由谁负责 | 备注 |
|---|---|---|
| HDF5 数据集名、维度顺序、属性 | **后端** | 算法侧最清楚自己的数据排布；前端硬编码会被反复改 |
| 算法节点之间传递 HDF5（产出/消费） | **后端** | 子进程内部完成，前端不掺和数据内容 |
| 前端预览/绘图时按窗口读子矩阵 | **前端** | 通过 `IH5Reader` 抽象，hyperslab 分块读，禁止全量 load |
| `IH5Reader` 默认实现（标准 HDF5） | **前端** | 能读 §6.1 这种约定结构 |
| 特殊数据集（稀疏/压缩/自定义编码）的读取器 | **后端** | 写一个 `IH5Reader` 子类，前端按 NodeMeta 注册项加载 |

> 一句话：**HDF5 内部由算法定义；前端只通过 `IH5Reader` 读取。**
> 如果你的节点用了非约定结构，请连同算法一起提供对应的 reader 子类。

---

## 7. 第一轮联调清单（建议明天就敲定）

| # | 节点 | 类型 | 谁出 EXE | 输入 | 输出 | 备注 |
|---|---|---|---|---|---|---|
| 1 | `input.las`        | PureOutput | 后端 | LAS 文件路径（params） | HDF5 | 把 LAS 读成统一 HDF5 |
| 2 | `preprocess.despike` | InOut    | 后端 | HDF5 | HDF5 | 去毛刺，参数 `threshold` |
| 3 | `preprocess.bandpass`| InOut    | 后端 | HDF5 | HDF5 | 带通，参数 `lowHz/highHz/order` |
| 4 | `display.line`     | PureInput  | —    | HDF5 | —    | 前端直接读 HDF5 画图 |
| 5 | `display.merge`    | PureInput  | —    | HDF5 (多端口) | — | 多曲线汇总 |

> 跑通 1→2→3→4 后，整条预处理链路就活了。

---

## 8. 我（前端）的承诺

- ✅ 节点拖拽、连线、保存模板/工程、撤销重做（部分）已经能用
- ✅ 跨进程算法调用框架（`ExternalProcessRunner`）已写完
- ✅ 全局日志面板（`LogDock`）已挂在编辑器底部，stderr/进度都会显示
- ✅ 双击节点能看输出文件信息；display 节点能画占位曲线
- ⏳ 把 `DemoNodes` 替换成"调真实 EXE"的节点 → **本周可做，等后端给第一个 EXE**
- ⏳ HDF5 解析（vcpkg 接 hdf5）→ 等第一个真实 HDF5 样例

---

## 9. 我需要后端今天/明天给我的东西

**P0（联调必需）**

- [ ] 一份"节点登记单"（用附录 A 的模板填，至少填 §7 表中那 3 个 EXE 节点）
- [ ] 第一个 EXE（哪怕是 echo 占位 EXE，能跑 JSON 协议就行）
- [ ] 一个样例 HDF5 文件（用来联调 PlotDialog 的解析）

**P1（一周内）**

- [ ] HDF5 输出格式确认（按 §6.1 还是另定）
- [ ] EXE 部署方式：是放在本程序 `bin/` 下，还是单独安装？路径如何配置？
- [ ] 是否需要环境变量 / DLL 依赖？（影响打包）

**P2（聊聊就行）**

- [ ] 错误码规范（`error.code` 字段要不要补）
- [ ] 大数据量节点是否需要流式 progress（每秒 N 次的频次约束）
- [ ] 是否需要"节点结果缓存"（同 params + 同输入 → 直接复用上次输出）

---

## 10. 双方明天对完后我会做的事

1. 把后端给的节点登记单转成 C++ `NodeMeta` 注册到 `NodeRegistry`，节点库里立刻能看到
2. 写一个通用的 `ExternalProcessNode : IProcessNode`，`run()` 直接转给 `ExternalProcessRunner`
3. 用后端给的样例 EXE + HDF5 跑通 §7 的那条链路
4. 把 `PlotDialog` 的 `setDemoFromSeed` 替换为读 HDF5 的真实数据

---

## 附录 A：节点登记单模板（请后端逐节点填一份）

```
typeId        :   ___________________________   （建议 group.subname，全局唯一）
displayName   :   ___________________________
category      :   ___________________________   （二级分类，节点库里分组）
group         :   DataInput / Preprocess / Interpret / Display
kind          :   PureOutput / InOut / PureInput

输入端口（无则填 ——）
  - key=____  显示名=____  format=____
  - key=____  显示名=____  format=____

输出端口（无则填 ——）
  - key=____  显示名=____  format=____

参数（按下面格式逐条填）
  名称           类型          默认值      合法范围/说明
  ___________   __________   __________   _________________
  ___________   __________   __________   _________________

EXE 路径（开发环境）：______________________________
依赖（DLL/数据）：__________________________________
样例输入：__________________________________________
样例输出：__________________________________________
描述（一句话）：____________________________________
```

---

## 附录 B：JSON 协议示例（最小可跑 EXE 伪代码）

```python
# echo_node.py - 假装是个算法 EXE
import sys, json, time

req = json.loads(sys.stdin.readline())
out = req.get("output", "out.h5")

# 假装算 5 步
for i in range(1, 6):
    print(json.dumps({"event":"progress","value":i*20,"message":f"step {i}/5"}), flush=True)
    time.sleep(0.2)

# 真正的算法做完了，写文件……（这里略）
print(json.dumps({"event":"done","outputs":[out]}), flush=True)
sys.exit(0)
```

打包成 `echo_node.exe` 后，前端节点 `params.exePath = "C:/.../echo_node.exe"` 就能调起来。

---

文档结束。明天有新约定再补本文档。
