# oilPro 工作流节点 JSON 规范 — NODE_SPEC_FOR_BACKEND

> **用途**：直接发给后端/算法同事，统一“节点怎么定义、工作流怎么描述、前端怎么调用 EXE”。
> **作者**：工程师 ly
> **状态**：当前拍板版本，可作为第一轮联调基线。

---

## 1. 一句话结论

`oilPro` 的工作流协议分三层：

1. **工作流 JSON**：描述这张图里有哪些节点、节点之间怎么连。
2. **节点定义 JSON**：描述每一种节点的输入端口、输出端口、参数、数据格式。
3. **运行时 JSON 协议**：前端真正调用后端 EXE 时，通过 `stdin/stdout` 传的内容。

---

## 2. 当前约定（已拍板）

### 2.1 节点命名

- 折线图节点统一命名为 `display.line`

原因：

- `display.line` 和 `input.las`、`preprocess.bandpass` 风格一致
- 更像稳定的节点类型 ID，而不是临时实现名
- 便于后续继续扩展 `display.heatmap`、`display.report`、`display.merge`

### 2.2 节点流向类型（kind）

| kind | 含义 | 典型例子 |
|---|---|---|
| `PureOutput` | 没有上游，只产出数据 | `input.las` |
| `InOut` | 有上游，处理后继续产出数据 | `preprocess.despike` / `preprocess.bandpass` |
| `PureInput` | 有上游，只消费数据，不再产出 | `display.line` / `display.merge` |

当前拍板：

- `display.line` 统一定义为 `PureInput`

这样定义的好处：

- 语义清晰：展示节点就是终点，不是中间处理节点
- 协议更简单：后端不用给展示节点定义输出文件
- 调度更稳定：前端不会误把展示节点继续串到后续算法链路

### 2.3 数据格式

项目内部处理链统一使用 `HDF5`：

- 输入节点：可以读 `LAS / CSV / ...`，但输出统一转成 `HDF5`
- 中间预处理节点：输入/输出都用 `HDF5`
- 展示节点：读取 `HDF5`

---

## 3. 工作流 JSON（描述节点流向）

这层 JSON 用来表达：

- 当前工作流里有哪些节点实例
- 每个节点实例的参数是什么
- 节点之间如何连线

### 3.1 推荐格式

```json
{
  "version": "oilpro.workflow.v1",
  "nodes": [
    {
      "id": "n1",
      "type": "input.las",
      "name": "LAS 数据输入",
      "x": 120,
      "y": 80,
      "params": {
        "path": "C:/data/demo.las"
      }
    },
    {
      "id": "n2",
      "type": "preprocess.despike",
      "name": "去毛刺",
      "x": 380,
      "y": 80,
      "params": {
        "threshold": 3.0
      }
    },
    {
      "id": "n3",
      "type": "preprocess.bandpass",
      "name": "带通滤波",
      "x": 640,
      "y": 80,
      "params": {
        "lowCut": 10.0,
        "highCut": 200.0,
        "order": 4
      }
    },
    {
      "id": "n4",
      "type": "display.line",
      "name": "折线图",
      "x": 900,
      "y": 80,
      "params": {}
    }
  ],
  "edges": [
    {
      "from": "n1",
      "fromPort": "out",
      "to": "n2",
      "toPort": "in"
    },
    {
      "from": "n2",
      "fromPort": "out",
      "to": "n3",
      "toPort": "in"
    },
    {
      "from": "n3",
      "fromPort": "out",
      "to": "n4",
      "toPort": "in"
    }
  ]
}
```

### 3.2 字段说明

#### `nodes[]`

每一项表示一个节点实例。

| 字段 | 类型 | 说明 |
|---|---|---|
| `id` | string | 该节点在当前工作流中的唯一实例 ID |
| `type` | string | 节点类型 ID，例如 `input.las` |
| `name` | string | 画布显示名，可由用户改名 |
| `x` | number | 节点在画布中的 x 坐标，仅前端布局使用 |
| `y` | number | 节点在画布中的 y 坐标，仅前端布局使用 |
| `params` | object | 当前节点参数值，运行时会原样下发给 EXE |

#### `edges[]`

每一项表示一条连线。

| 字段 | 类型 | 说明 |
|---|---|---|
| `from` | string | 上游节点实例 ID |
| `fromPort` | string | 上游输出端口 key |
| `to` | string | 下游节点实例 ID |
| `toPort` | string | 下游输入端口 key |

### 3.3 后端需要知道什么

后端通常**不需要直接消费整张工作流 JSON**，因为前端会负责：

- DAG 校验
- 拓扑调度
- 节点间文件传递
- 逐节点调用 EXE

但后端需要理解这层 JSON 的含义，因为：

- `type` 决定调用哪个节点 EXE
- `fromPort/toPort` 决定输入文件如何映射到 `inputs`
- `params` 会直接成为运行时请求的一部分

---

## 4. 节点定义 JSON（描述节点画像）

这层 JSON 用来表达“某一种节点长什么样”。  
建议前后端按这个格式对齐节点登记信息。

### 4.1 推荐格式

```json
{
  "version": "oilpro.node-spec.v1",
  "nodes": [
    {
      "typeId": "input.las",
      "displayName": "LAS 数据输入",
      "category": "数据输入",
      "group": "DataInput",
      "kind": "PureOutput",
      "inputs": [],
      "outputs": [
        {
          "key": "out",
          "displayName": "数据",
          "format": "HDF5"
        }
      ],
      "params": [
        {
          "key": "path",
          "displayName": "LAS文件路径",
          "type": "file",
          "required": true,
          "default": "",
          "accept": [".las"],
          "description": "输入 LAS 文件路径"
        }
      ],
      "description": "从 LAS 文件读取测井曲线并转换为内部 HDF5。",
      "externalProcess": true
    },
    {
      "typeId": "preprocess.despike",
      "displayName": "去毛刺",
      "category": "DAS 处理",
      "group": "Preprocess",
      "kind": "InOut",
      "inputs": [
        {
          "key": "in",
          "displayName": "输入",
          "format": "HDF5"
        }
      ],
      "outputs": [
        {
          "key": "out",
          "displayName": "输出",
          "format": "HDF5"
        }
      ],
      "params": [
        {
          "key": "threshold",
          "displayName": "阈值",
          "type": "number",
          "required": true,
          "default": 3.0,
          "minimum": 0.0,
          "description": "超过该阈值的尖峰视为异常点"
        }
      ],
      "description": "基于阈值滤除尖峰噪声。",
      "externalProcess": true
    },
    {
      "typeId": "preprocess.bandpass",
      "displayName": "带通滤波",
      "category": "DAS 处理",
      "group": "Preprocess",
      "kind": "InOut",
      "inputs": [
        {
          "key": "in",
          "displayName": "输入",
          "format": "HDF5"
        }
      ],
      "outputs": [
        {
          "key": "out",
          "displayName": "输出",
          "format": "HDF5"
        }
      ],
      "params": [
        {
          "key": "lowCut",
          "displayName": "低截止频率",
          "type": "number",
          "required": true,
          "default": 10.0,
          "minimum": 0.0,
          "unit": "Hz"
        },
        {
          "key": "highCut",
          "displayName": "高截止频率",
          "type": "number",
          "required": true,
          "default": 200.0,
          "minimum": 0.0,
          "unit": "Hz"
        },
        {
          "key": "order",
          "displayName": "滤波阶数",
          "type": "integer",
          "required": false,
          "default": 4,
          "minimum": 1
        }
      ],
      "description": "Butterworth 带通滤波。",
      "externalProcess": true
    },
    {
      "typeId": "display.line",
      "displayName": "折线图",
      "category": "可视化",
      "group": "Display",
      "kind": "PureInput",
      "inputs": [
        {
          "key": "in",
          "displayName": "数据",
          "format": "HDF5"
        }
      ],
      "outputs": [],
      "params": [],
      "description": "读取 HDF5 结果并在前端显示。",
      "externalProcess": false
    },
    {
      "typeId": "display.merge",
      "displayName": "多结果汇总",
      "category": "可视化",
      "group": "Display",
      "kind": "PureInput",
      "inputs": [
        {
          "key": "in1",
          "displayName": "结果1",
          "format": "HDF5"
        },
        {
          "key": "in2",
          "displayName": "结果2",
          "format": "HDF5"
        },
        {
          "key": "in3",
          "displayName": "结果3",
          "format": "HDF5"
        }
      ],
      "outputs": [],
      "params": [],
      "description": "前端汇总多个上游结果并展示。",
      "externalProcess": false
    }
  ]
}
```

### 4.2 字段说明

| 字段 | 类型 | 说明 |
|---|---|---|
| `typeId` | string | 全局唯一节点类型 ID，建议 `group.subname` 风格 |
| `displayName` | string | 节点在节点库/画布上的显示名 |
| `category` | string | 节点库二级分类 |
| `group` | string | `DataInput / Preprocess / Interpret / Display` |
| `kind` | string | `PureOutput / InOut / PureInput` |
| `inputs` | array | 输入端口定义，`PureOutput` 时可为空 |
| `outputs` | array | 输出端口定义，`PureInput` 时可为空 |
| `params` | array | 参数定义，用于生成表单并传给 EXE |
| `description` | string | 节点说明 |
| `externalProcess` | boolean | 是否通过外部 EXE 执行 |

#### 端口对象

```json
{
  "key": "in",
  "displayName": "输入",
  "format": "HDF5"
}
```

字段说明：

- `key`：端口唯一键，运行时会用它组织 `inputs`
- `displayName`：前端显示名
- `format`：数据格式，用于连线校验

#### 参数对象

```json
{
  "key": "threshold",
  "displayName": "阈值",
  "type": "number",
  "required": true,
  "default": 3.0,
  "minimum": 0.0,
  "description": "超过该阈值的尖峰视为异常点"
}
```

推荐支持的 `type`：

- `string`
- `number`
- `integer`
- `boolean`
- `file`
- `enum`

---

## 5. 运行时 JSON 协议（前端调用 EXE）

前端不会把整张 DAG 一次性交给后端。  
前端会按调度顺序，**一次调用一个节点 EXE**。

### 5.1 前端 → EXE（stdin，一行 JSON）

```json
{
  "op": "run",
  "params": {
    "threshold": 3.0
  },
  "inputs": {
    "in": ["C:/temp/oilpro/upstream.h5"]
  },
  "output": "C:/temp/oilpro/node-n2.h5"
}
```

字段说明：

| 字段 | 类型 | 说明 |
|---|---|---|
| `op` | string | 当前固定为 `run` |
| `params` | object | 当前节点参数，来自工作流 JSON 中该节点的 `params` |
| `inputs` | object | 按输入端口 key 组织的文件列表 |
| `output` | string | 前端建议的输出路径 |

说明：

- `inputs` 的 key 必须与节点定义里的 `inputs[].key` 对齐
- 即使一个端口通常只有一个文件，也统一用数组，方便未来扩展多输入
- 算法可以使用前端给的 `output`，也可以自行决定真实输出路径

### 5.2 EXE → 前端（stdout，每行一条 JSON）

#### 进度事件

```json
{"event":"progress","value":25,"message":"loading input"}
```

#### 日志事件

```json
{"event":"log","message":"threshold=3.0"}
```

#### 成功事件

```json
{"event":"done","outputs":["C:/temp/oilpro/node-n2.h5"]}
```

#### 失败事件

```json
{"event":"error","message":"file not found"}
```

### 5.3 约束

- `progress.value` 范围为 `0 ~ 100`
- 成功时建议发送一次 `done`
- `done.outputs` 为真实输出文件路径数组
- 失败时发送 `error` 后应退出非 0
- `stderr` 可输出普通文本，前端会记录到日志面板

### 5.4 退出码

| exit code | 含义 |
|---|---|
| `0` | 成功 |
| 非 `0` | 失败 |

前端当前有一个兼容策略：

- 如果 EXE 退出码是 `0`，但没有发送 `done`
- 前端会临时容错视为成功，并使用请求里的 `output` 作为默认输出路径

但正式联调时，仍然建议后端**明确发送 `done`**。

---

## 6. 第一轮联调建议节点

建议先只对齐下面 5 个节点，先跑通一条完整链路：

| 节点 | kind | 是否后端 EXE | 输入 | 输出 |
|---|---|---|---|---|
| `input.las` | `PureOutput` | 是 | LAS 文件路径（参数） | HDF5 |
| `preprocess.despike` | `InOut` | 是 | HDF5 | HDF5 |
| `preprocess.bandpass` | `InOut` | 是 | HDF5 | HDF5 |
| `display.line` | `PureInput` | 否 | HDF5 | 无 |
| `display.merge` | `PureInput` | 否 | 多路 HDF5 | 无 |

第一条联调链路建议：

```text
input.las -> preprocess.despike -> preprocess.bandpass -> display.line
```

---

## 7. HDF5 输出建议

为了让前端后续直接读取结果并展示，建议后端输出的 HDF5 尽量遵循下面结构：

```text
/data
    /x
    /y
    /series/0
    /series/1
/meta
    /unit_x
    /unit_y
    /description
```

约定建议：

- `display.line`：主要读取 `/data/x` 和 `/data/y`
- `display.merge`：主要读取 `/data/series/*`

如果后端最终 HDF5 结构要改，也可以，但请尽早统一。

---

## 8. 后端本轮需要给前端的东西

### P0

- 每个算法节点一份节点定义 JSON
- 每个算法节点对应的 EXE
- 一组样例输入
- 一组期望输出

### P1

- 确认 EXE 部署路径
- 确认是否依赖 DLL / 环境变量
- 确认 HDF5 结构细节

---

## 9. 建议直接回复的最小口径

简单回复后端“描述节点流向的 json 格式会长什么样”：

```text
我们现在准备把协议分两层：

1. 工作流 JSON：描述节点实例和边，也就是这张 DAG 怎么连
2. 运行时 JSON：前端按 DAG 调度后，一次调用一个节点 EXE

工作流 JSON 示例：
{
  "version": "oilpro.workflow.v1",
  "nodes": [
    {
      "id": "n1",
      "type": "input.las",
      "name": "LAS 数据输入",
      "x": 120,
      "y": 80,
      "params": { "path": "C:/data/demo.las" }
    },
    {
      "id": "n2",
      "type": "preprocess.despike",
      "name": "去毛刺",
      "x": 380,
      "y": 80,
      "params": { "threshold": 3.0 }
    }
  ],
  "edges": [
    {
      "from": "n1",
      "fromPort": "out",
      "to": "n2",
      "toPort": "in"
    }
  ]
}

真正执行时，前端不会把整张图直接交给单个 EXE，而是按节点逐个调：
{
  "op": "run",
  "params": { "threshold": 3.0 },
  "inputs": { "in": ["C:/temp/oilpro/upstream.h5"] },
  "output": "C:/temp/oilpro/node-n2.h5"
}
```

---


