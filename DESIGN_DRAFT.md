# 工作流节点引擎 — 初版架构提案（讨论稿）

> **状态**：初版 / 待评审
> **作者**：工程师 ly（前端）
> **目的**：明天和后端 / 算法同事对一下方向，先把"节点引擎"这件事的设计原则、模块边界、调用方式定下来，再去具体实现。

---

## 1. 我们要解决的问题

桌面端给地质 / 测井工程师一个"画布式"的预处理工具：

- 用户拖拽节点连成 DAG
- 节点种类：数据输入 → 预处理 → 解释 → 展示
- 支持"单节点跑 / 全部跑 / 停止"
- 支持把搭好的 DAG 保存为**模板**（无数据）和**工程**（含数据引用）
- 一个节点崩溃，**不能拖垮主程序**

主要矛盾：**算法实现是后端 / 算法组的事**，前端不应该把算法代码吃进自己的进程，更不能让算法 bug 导致 GUI 崩。

---

## 2. 核心设计原则（请评审）

| # | 原则 | 解释 |
|---|---|---|
| ① | **算法 = 独立进程** | 一个节点跑一次，启动一个外部 EXE，跑完就退；GUI 进程不动算法代码 |
| ② | **进程间用文本协议** | 用 stdin/stdout 一行一条 JSON 通信；不引入 RPC / gRPC，部署 0 依赖 |
| ③ | **统一中间格式 = HDF5** | 节点之间传文件路径，不传内存对象；除"输入节点"外都吃 HDF5 吐 HDF5 |
| ④ | **节点元数据驱动 UI** | 节点的端口、参数、分类全部由元数据声明，UI 完全通用，加新节点不用改 UI |
| ⑤ | **接口 / 实现严格分离** | GUI 只依赖一组纯接口（IWorkflowEngine / IProcessNode 等），便于 mock 测试和替换实现 |

---

## 3. 模块划分（前端视角）

```
┌─────────────────────────────────────────────────────────┐
│ GUI 层（Qt Widgets）                                     │
│   - 画布 / 节点库 / 工具栏 / 属性面板 / 日志面板          │
│   - 只调接口，不知道算法在哪里                            │
└─────────────────────────────────────────────────────────┘
                       │
                       ▼ 接口
┌─────────────────────────────────────────────────────────┐
│ 引擎层 (WorkflowEngine)                                  │
│   - DAG 维护（增删节点 / 增删边 / 校验环）                │
│   - 拓扑排序 + 同级并发调度（QThreadPool）                │
│   - 单节点跑 / 全部跑 / 停止                              │
│   - 序列化（模板 / 工程）                                  │
└─────────────────────────────────────────────────────────┘
                       │
                       ▼ 调
┌─────────────────────────────────────────────────────────┐
│ 节点实现 (IProcessNode 实现)                             │
│   - 真实的算法节点 = 一个壳：内部启动外部 EXE             │
│   - 数据格式转换节点 = 内置 C++ 实现（少数）              │
└─────────────────────────────────────────────────────────┘
                       │
                       ▼ 启动
              ┌────────────────────┐
              │  算法 EXE (后端)    │
              │  stdin: 任务 JSON   │
              │  stdout: 事件 JSON  │
              └────────────────────┘
```

---

## 4. 关键接口草案（征求意见）

### 4.1 节点元数据（NodeMeta）

```cpp
struct NodeMeta {
    QString          typeId;        // 全局唯一 ID, 如 "preprocess.bandpass"
    QString          displayName;   // 中文显示名
    QString          category;      // 二级分类（节点库分组）
    NodeGroup        group;         // DataInput / Preprocess / Interpret / Display
    NodeKind         kind;          // PureOutput / InOut / PureInput
    QList<PortSpec>  inputs;        // 输入端口
    QList<PortSpec>  outputs;       // 输出端口
    QVariantMap      defaultParams; // 默认参数
    QString          description;
    bool             externalProcess;  // 是否走外部 EXE
};
```

> **讨论点**：参数描述要不要再丰富些（带类型 / 范围 / 单位）？我先用 QVariantMap，后面可以演进成 `ParamSpec` 列表。

### 4.2 节点运行接口（IProcessNode）

```cpp
class IProcessNode {
public:
    virtual NodeRunResult run(ProgressCallback onProgress) = 0;
    virtual void requestCancel() = 0;
    virtual ~IProcessNode() = default;
};
```

> **讨论点**：取消的语义如何约定？我倾向 GUI 发 terminate，5 秒不退就 kill，后端在长循环里能 catch 信号最好。

### 4.3 引擎接口（IWorkflowEngine）

```cpp
class IWorkflowEngine : public QObject {
    Q_OBJECT
public:
    // DAG 维护
    virtual QString addNode(const QString& typeId, QPointF pos) = 0;
    virtual bool    addEdge(const EdgeInstance& e) = 0;     // 自动校验端口格式
    virtual void    runSingle(const QString& id) = 0;
    virtual void    runAll() = 0;
    virtual void    stop() = 0;

    // 序列化
    virtual QByteArray serialize(bool includeData) = 0;     // 模板 / 工程
    virtual bool       deserialize(const QByteArray& d) = 0;

signals:
    void nodeStatusChanged(QString id, NodeStatus s);
    void nodeProgress(QString id, int pct, QString msg);
    void nodeFinished(QString id, NodeRunResult r);
};
```

---

## 5. 跨进程调用协议（征求后端意见）

**单任务一进程**模型，进程结束即任务结束。

### 父进程 → 子进程（stdin，一行 JSON）

```json
{
  "op":     "run",
  "params": {"...节点参数..."},
  "inputs": {"in": ["A.h5"], "in2": ["B.h5"]},
  "output": "C:/temp/oilpro/<nodeId>.h5"
}
```

### 子进程 → 父进程（stdout，每行一条 JSON）

```json
{"event":"progress","value":42,"message":"step 3/8"}
{"event":"log","message":"normalize done"}
{"event":"done","outputs":["C:/temp/oilpro/<nodeId>.h5"]}
{"event":"error","message":"input file missing"}
```

**stderr** = 自由文本，前端会汇总到日志面板（适合算法 printf 调试）。
**exit code** = 0 成功 / 非 0 失败。

> **讨论点**：
> - 进度回报频率上限？（避免每秒上千次刷爆 GUI）
> - 错误码体系要不要独立定义（`error.code` 字段）？
> - 是否需要子进程支持 `op:"cancel"` 这种主动指令？还是 SIGTERM 就够？

---

## 6. 数据格式约定

| 位置 | 格式 |
|---|---|
| 节点之间传输 | **HDF5 文件路径**（不传内存对象） |
| 数据输入节点的输入 | LAS / TDMS / SEGY / CSV / … |
| 所有中间结果 | HDF5 |
| 展示节点输入 | HDF5 |

> **讨论点**：HDF5 的内部 schema 要不要约束？我倾向定一份"曲线类数据"的最小 schema（`/data/x`、`/data/y` 或 `/data/series/*`），其它字段算法自由扩展。这样 GUI 的展示节点就能有通用读取实现。

---

## 7. 并发与调度

- 引擎用 `QThreadPool`，**同一拓扑层**的无依赖节点并发跑
- 上游全部成功后，下游才被调度
- 任意节点失败 → 下游标 `Skipped`（**待定**：还是直接停止整条流？）
- "停止" → 给所有正在跑的节点发 cancel，等它们退出后整体 idle

> **讨论点**：失败传播策略你们倾向哪种？

---

## 8. 节点登记单（让后端按这个填）

每加一个节点，后端给我一份：

```
typeId        :   ___________________________
displayName   :   ___________________________
category      :   ___________________________
group         :   DataInput / Preprocess / Interpret / Display
kind          :   PureOutput / InOut / PureInput

输入端口（无则填 ——）
  - key=____  显示名=____  format=____

输出端口
  - key=____  显示名=____  format=____

参数
  名称 / 类型 / 默认值 / 范围

EXE 路径：______________________________
样例输入 / 输出：______________________________
描述：______________________________
```

明天对完后我做的事：
- 把这些登记单转成 NodeMeta 注册到节点库
- 写一个通用的"调外部 EXE 的节点壳子"，所有外部算法节点共享
- 跑通一条端到端链路（输入 → 1~2 个预处理 → 展示）

---

## 9. 待和后端讨论的开放问题（汇总）

1. **算法 EXE 部署方式**：放主程序 `bin/` 下？还是单独安装？路径配置由谁管？
2. **HDF5 schema**：要不要统一一份最小约定，还是每个节点自由发挥？
3. **进度 / 日志频率**：是否需要约束上限？
4. **错误码体系**：是否需要带 code，还是仅 message？
5. **取消信号语义**：terminate 够不够？需不需要 `op:"cancel"`？
6. **失败传播**：单节点失败 → 后续节点跳过 / 整体停止？
7. **节点缓存**：相同输入 + 相同参数是否复用上次输出？
8. **第一轮要先实现哪几个节点**？我建议：LAS 输入 / 去毛刺 / 带通滤波 / 折线展示

---

## 10. 下一步（如果今天对方向都认同）

1. 我把上述接口在前端实现一版骨架（**约 1~2 周**）
2. 后端给一个 echo EXE（按 §5 协议实现，10 行代码）+ 一份节点登记单
3. 联调，跑通最小链路
4. 之后按节点登记单批量加节点

---

文档结束，等明天讨论后定版。
