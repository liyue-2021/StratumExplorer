# oilPro 前后端对接备忘 — BACKEND_HANDOFF

> **场合**：和后端/算法同事对「前端怎么调 EXE、要提供什么」。
> **作者**：工程师 ly  
> **前端范围**：DAG、属性面板、生成任务配置文件、启动 EXE、读任务结果 JSON。**不**负责业务 HDF5 数据集读写。

---

## 1. 一句话定位

桌面端负责**画工作流、填参数、写任务配置、启动算法 EXE、读任务结果**。

算法计算与**业务 HDF5** 的读写由后端 EXE 完成。

```
   ┌─────────── 桌面端 (Qt/C++) ───────────┐
   │  WorkflowEditor + 属性面板             │
   │       ▼                                │
   │  WorkflowEngine                        │
   │       ▼                                │
   │  ExternalProcessNode                   │
   │       ▼                                │
   │  ExternalProcessRunner                 │
   │    写 input_config_<taskId>.json       │
   │    启动: exe.exe <config.json>         │
   │    读:   <output>/task_<taskId>.json   │
   └────────────────────────────────────────┘
                    │
                    ▼
              后端算法 EXE
         （读配置、读/写业务 .h5、写结果 JSON）
```

环境变量 `OILPRO_PROTOCOL=JSON`（默认构建）；若双方约定真 HDF5 配置文件，则为 `HDF5` 且扩展名为 `.h5`（需 CMake 开启 `USE_HDF5`）。

---

## 2. 调用方式（当前默认）

| 步骤 | 谁做 | 说明 |
|------|------|------|
| 1 | 前端 | 在 EXE 工作目录或 `outputPath` 下生成 `input_config_<taskId>.json` |
| 2 | 前端 | `exe.exe <config.json>`（**唯一命令行参数**） |
| 3 | 后端 | 解析 JSON，执行算法，写出业务数据 HDF5（路径自定） |
| 4 | 后端 | 在 `io.output_path` 目录写 `task_<taskId>.json`，或向 stdout 打印结果文件绝对路径 |
| 5 | 前端 | 读 `task_<taskId>.json` 取 `status` / `data_file_path` 等 |

---

## 3. input_config JSON 结构

```json
{
  "task_info": {
    "task_id": 12345,
    "func_id": 1,
    "func_name": "func_depth_correct"
  },
  "io": {
    "input_path": ["D:/data/in.h5"],
    "output_path": "C:/Temp/oilpro/",
    "ref_depth_path": "",
    "calib_path": "",
    "temp_path": "",
    "ref_path": ""
  },
  "params": {
    "interp_method": "linear",
    "depth_step": 0.5
  }
}
```

### 3.1 六个 IO 路径（后端按需使用）

| JSON 字段 | TaskRequest 成员 | 用途 |
|-----------|------------------|------|
| `io.input_path` | `inputPaths` | 上游传入的业务数据文件列表 |
| `io.output_path` | `outputPath` | 输出目录（结果 JSON + 建议的数据产物目录） |
| `io.ref_depth_path` | `refDepthPath` | 深度矫正参考深度 |
| `io.calib_path` | `calibPath` | DTS 温度标定 |
| `io.temp_path` | `tempPath` | DSS 温耦分离同步温度 |
| `io.ref_path` | `refPath` | DSS 基线校准参考 |

节点 `params` 里若带上述 path 键，前端会在组装请求时 `take` 到对应 IO 字段（见 `ExternalProcessNode.cpp`）。

### 3.2 task_info

| 字段 | 说明 |
|------|------|
| `task_id` | 实例级任务 ID（当前为节点 instanceId 的 hash） |
| `func_id` | `0~41`，与 `ProductionNodes` 注册一致 |
| `func_name` | 如 `func_depth_correct` |

### 3.3 params

属性面板全部算法参数（已去掉 `exePath`）。类型：string / number / bool / 数组。

---

## 4. task 结果 JSON 结构（后端写入）

```json
{
  "status": { "code": 0, "message": "ok", "error": "" },
  "io": {
    "data_file_path": "D:/out/result.h5",
    "output_path": ["D:/out/result.h5"]
  },
  "task_info": { "task_id": 12345, "func_id": 1, "func_name": "func_depth_correct" },
  "time": { "duration": 1.23 }
}
```

| 字段 | 说明 |
|------|------|
| `status.code` | `0` 成功，非 0 失败 |
| `io.data_file_path` | 主业务产出 HDF5（前端展示/下游连线用） |
| `io.output_path` | 可选，多文件列表 |

---

## 5. 节点元数据（前端已注册 42 个）

- 注册代码：`ProductionNodes.cpp`
- 属性面板：`node_client_params.json`（23 个真实参数 + 19 个虚构占位，`fictional: true`）
- `node_specs.json`：**仅文档**，程序不读

后端需提供：

| 项 | 说明 |
|----|------|
| EXE | 符合上文 JSON 协议 |
| func_id / func_name | 与前端 `NodeMeta` 一致 |
| 样例 input_config + task 结果 | 联调 |

---

## 6. 业务 HDF5（后端负责）

前端**不**解析算法产出的 HDF5 内部结构。请后端：

1. 约定 `/data/x`、`/data/y` 等数据集布局（见旧版 §6.1，仍可沿用）
2. 在 `task` 结果里填 `data_file_path`
3. 若结构特殊，另提供读取说明或后续 `IH5Reader` 插件

---

## 7. 旧版 stdin/stdout JSON 协议（已废弃）

早期设计为一行 stdin + 多行 stdout 事件流，**当前代码已改为配置文件模式**。若 EXE 仍按旧协议实现，需后端改版或单独分支。

<details>
<summary>旧协议摘要（仅供迁移参考）</summary>

stdin 一行：`{"op":"run","params":{},"inputs":{},"output":"..."}`  
stdout 事件：`progress` / `done` / `error`

</details>

---

## 8. 附录：节点登记单（后端填写）

```
typeId        : preprocess.depth_correct
func_id       : 1
func_name     : func_depth_correct
EXE 路径      : _______________________
样例 input    : _______________________
样例 output   : _______________________
```

---

文档结束。协议以 `ExternalProcessRunner.h` 注释为准。
