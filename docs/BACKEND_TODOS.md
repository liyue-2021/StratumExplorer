# 后端待办索引（给联调同事）

> 前端（GUI）只负责界面、工作流编排、写任务配置、启动 EXE、读结果 JSON。  
> **算法与业务 HDF5 读写由后端对接甲方 EXE 完成。**  
> 协议总览：`BACKEND_HANDOFF.md`

---

## 优先对接（当前最小工作流）

| 优先级 | 节点 | typeId | 代码位置 | 说明 |
|--------|------|--------|----------|------|
| P0 | 数据格式转换 | `preprocess.format_convert` | `ExternalProcessNode.cpp` → `TODO(后端)` | 现为占位写 `.h5`；建议 `externalProcess=true` + EXE `func_format_convert` |
| P1 | 深度矫正等 | `preprocess.depth_correct` … | `ExternalProcessRunner.cpp` | 已有 EXE 协议，实现对应 `func_id` / `func_name` |

**数据输入** `input.data_input`：前端只传 LAS 路径，一般无需 EXE。

---

## 代码内 `TODO(后端)` / `TODO: 对接`

在仓库根目录执行：

```powershell
rg "TODO\(后端\)|TODO: 对接" src/
```

节点参数均在**右侧属性面板**编辑（`WorkflowEditorTab::rebuildParamForm`）；`input.data_input` / `preprocess.format_convert` 有专用面板逻辑。

---

## 登记

| 节点 | func_id | func_name | EXE 路径 | 负责人 |
|------|---------|-----------|----------|--------|
| 格式转换 | 0 | func_format_convert | | |
| 深度矫正 | 1 | func_depth_correct | | |

（完整 42 节点见 `ProductionNodes.cpp`）
