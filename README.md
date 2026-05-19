# StratumExplorer（oilPro）

测井数据预处理工作流桌面端（C++ / Qt6）。

## 依赖

- C++17、Qt 6.10+、CMake 3.26+、MSVC 2022

## 快速开始

```powershell
# Debug 构建（日志：tools/build/logs/build_oilpro.log）
cmd /c tools\build\build_oilpro.cmd

# Release 构建
cmd /c tools\build\build_release.cmd
```

产物：`bin\windows\x64\DEBUG\StratumExplorer.exe`（Debug）或对应 Release 目录。

环境自检：`powershell -File tools\verify_environment.ps1`

## 仓库布局

```txt
CMakeLists.txt      # CMake 工程入口（必须在根目录）
README.md           # 本文件
cmake/              # CMake 模块
docs/               # 设计、对接、进度文档 → 见 docs/README.md
tools/              # 构建脚本、环境检查
src/                # 源代码（app / core / gui）
resources/          # 图标、样式、字体
translations/       # 多语言
third_party/        # 第三方库
platform/           # 平台适配
test/               # 测试（可选 WITH_TEST）
```

## 文档

| 用途 | 路径 |
|------|------|
| 架构与约定 | [docs/PROJECT_CONTEXT.md](docs/PROJECT_CONTEXT.md) |
| 发版 / 待办 | [docs/HANDOFF_CHECKLIST.md](docs/HANDOFF_CHECKLIST.md) |
| 后端 EXE 对接 | [docs/BACKEND_HANDOFF.md](docs/BACKEND_HANDOFF.md) |
| 换机配置 | [docs/MIGRATION_GUIDE.md](docs/MIGRATION_GUIDE.md) |

节点属性面板运行时配置：`src/core/service/processing/node_client_params.json`

## 说明

- `depend/`、`scripts/`：本地甲方资料与临时脚本，已 gitignore，不进仓库。
- 根目录请勿再放构建日志、`.db` 运行时库；构建输出在 `build/`、`bin/`（已 ignore）。
