# 节点配置弹窗（.ui 方式）

## 约定（2026-05 甲方变更）

- **参数以 `node_client_params.json`（甲方属性参数表）为准**，在右侧属性面板编辑。
- 原节点样例 `.ui` 弹窗**保留不删**，但 `configDialog` 已移除，**双击不再打开**（见 `WorkflowEditorTab::onNodeDoubleClicked`）。
- **数据输入** `input.data_input`：在属性面板多选 LAS，不用独立 `.ui`。

| 文件 | 职责 |
|------|------|
| `XxxDialog.ui` | 样例弹窗（待甲方确认后是否启用） |
| `XxxDialog.h/cpp` | 同上 |

**布局约定**（样例弹窗）：无 `QGroupBox`；四列/六列网格；标签左对齐；参考 `DataMergeDialog.ui` / `LfdasExtractDialog.ui`。

## 已有

- `FormatConvertDialog.ui` — 数据格式转换（`preprocess.format_convert`）
- `DepthCorrectDialog.ui` — 深度矫正（`preprocess.depth_correct`）
- `TimeCorrectDialog.ui` — 时间矫正（`preprocess.depth_time_correct`）
- `DataCropDialog.ui` — 数据裁剪（`preprocess.data_crop`）
- `DataMergeDialog.ui` — 数据合并（`preprocess.data_merge`）
- `DasConvertDialog.ui` — DAS数据转换（`preprocess.das_convert`）
- `LfdasExtractDialog.ui` — LF-DAS提取（`preprocess.lfdas_extract`，**六列网格**每行 3 组参数，窗口加宽、无滚动区）

## 新增节点弹窗步骤

1. 复制 `FormatConvertDialog.ui` / `.h` / `.cpp` 为模板，改 `class` 名与控件名。
2. 在 `CMakeLists.txt` 的 `FORMS` 里加入 `XxxDialog.ui`。
3. 在 `node_client_params.json` 设置 `"propertyPanel": "none"`, `"configDialog": "xxx"`。
4. 在 `WorkflowEditorTab::onNodeDoubleClicked` 里按 `configDialogId` 打开对应对话框。

`CMAKE_AUTOUIC` 已在工程全局开启，会自动生成 `ui_XxxDialog.h`。

## 高 DPI / 多分辨率

- 入口：`src/app/main.cpp` 在 `QApplication` 构造前调用 `setupHiDpiBeforeQApplication()`（见 `HiDpi.h`）。
- Qt 6 已默认高 DPI 缩放，勿再使用已移除的 `AA_EnableHighDpiScaling`。
- 弹窗用 `.ui` 里的 **QVBoxLayout / QHBoxLayout / QGridLayout**，避免代码里 `setGeometry`；`geometry`/`minimumSize` 仅作初始尺寸提示。
- 样式表里的 `min-height: 28px` 为控件最小触控高度，会随系统缩放；字号优先 `pt` 或 `setPointSize`，避免 `setPixelSize`。
- 图表：`PlotDialog` 中 `QCustomPlot` 使用 `QSizePolicy::Expanding` 并放入布局 `stretch=1`。
