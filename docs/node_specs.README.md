# node_specs.json 说明

**程序运行时不读取本文件。**

| 用途 | 实际来源 |
|------|----------|
| 节点库、端口、`funcId`/`funcName` | `src/core/service/processing/ProductionNodes.cpp` |
| 属性面板参数（中文名、默认值、顺序） | `src/core/service/processing/node_client_params.json` |

`docs/node_specs.json` 仅作人工查阅、与甲方文档对照；修改面板参数请改 `src/core/service/processing/node_client_params.json` 后重新编译。
