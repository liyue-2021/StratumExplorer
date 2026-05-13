# oilPro 新电脑搬迁与接力指南 — MIGRATION_GUIDE

> **场景**：把项目从 A 电脑搬到 B 电脑，让 B 电脑上的 AI 能继续干活。
> **作者**：工程师 ly
> **本指南配合**：`PROJECT_CONTEXT.md`（架构）+ `BACKEND_HANDOFF.md`（前后端约定）一起看。

---

## 0. 给新窗口 AI 的开场白（直接粘贴）

```text
你接手一个 Qt 6.10.3 + MSVC2022 的 C++ 桌面项目 oilPro。
项目路径请看本电脑实际放在哪里（一般在用户的 Downloads 或 D 盘）。

请按这个顺序读文档：
1. MIGRATION_GUIDE.md  — 本电脑环境配置与构建（最先看）
2. PROJECT_CONTEXT.md  — 整体架构、约定
3. BACKEND_HANDOFF.md  — 与后端的接口协议

执行顺序：
A. 按 MIGRATION_GUIDE §3 检查/补齐环境
B. 按 §4 改路径变量
C. 按 §5 跑首次构建，确认 EXITCODE=0
D. 然后再做我具体让你做的事

这个项目最近一次成功构建是 Phase D（保存路径记忆 + ExternalProcessRunner +
LogBus + LogDock + PlotDialog 通用化）。代码里我新加的头文件都有 "作者：
工程师 ly" 注释，看到这种就知道是这一轮加的。
```

---

## 1. 必装软件清单

| 软件 | 版本 | 装哪里（本机示例） | 是否必需 |
|---|---|---|---|
| Qt | **6.10.3 MSVC2022 64-bit** | `C:\Qt\6.10.3\msvc2022_64` | **必需** |
| Qt 自带 CMake | 3.30+ | `C:\Qt\Tools\CMake_64\bin\cmake.exe` | **必需**（不要用系统 4.0） |
| Qt 自带 jom | — | Qt 维护工具会一起装 | **必需** |
| Visual Studio 2022 Community | 17.x，含 C++ 桌面开发负载 | `C:\Program Files\Microsoft Visual Studio\2022\Community` | **必需**（编译器 + vcvars） |
| Qt Creator | 任意 | 同 Qt 安装目录 | 可选（IDE 备选） |
| VS Code + C/C++ 扩展 | 任意 | — | 可选（推荐） |
| 7-Zip | 任意 | — | 可选（解压用） |

### Qt 没勾 CMake 怎么办

打开 `C:\Qt\MaintenanceTool.exe` → "Add or remove components" → 勾 `Developer and Designer Tools → CMake 3.30.x 64-bit`（顺便勾 `Ninja`）→ 安装。

完成后应该能看到 `C:\Qt\Tools\CMake_64\bin\cmake.exe`。

### CMake 装在 Qt 目录外（独立安装也可以）

CMake 是独立工具，**装在哪都行**，跟 Qt 无关。例如直接从 [cmake.org](https://cmake.org/download/) 下到 `C:\Program Files\CMake\` 完全可以用。两点要求：

1. **版本必须 ≥ 3.21 且 < 4.0**。本项目的 `cmake_minimum_required` 用 3.x 语法，CMake 4.x 移除了部分旧策略可能编译失败。推荐 3.28 / 3.29 / 3.30。
2. **能被脚本调用**。要么安装时勾 *Add CMake to the system PATH*，要么在 `build_oilpro.cmd` 里把 `set CMAKE=...` 改成你电脑上的实际路径，例如：
   ```bat
   set CMAKE=C:\Program Files\CMake\bin\cmake.exe
   ```

CMake 通过命令行参数 `-DCMAKE_PREFIX_PATH=%QT_DIR%` 找 Qt，跟 cmake.exe 自己装在哪没有任何关系。

---

## 2. 解压源码

把 `oilPro_handoff.zip` 或 `.7z` 解到一个你喜欢的路径，**不要带空格、不要带中文**。
本指南示例假设解到：`C:\dev\oilPro\`。

解完后目录结构应该长这样：
```
C:\dev\oilPro\
  CMakeLists.txt
  PROJECT_CONTEXT.md
  BACKEND_HANDOFF.md
  MIGRATION_GUIDE.md  ← 你正在看的
  cmake/
  src/
  third_party/
  ...
```

---

## 3. 检查环境（5 分钟）

打开 PowerShell，逐行跑下面命令，**确认每一项都有输出**：

```powershell
# 1. Qt 在不在
Test-Path "C:\Qt\6.10.3\msvc2022_64\bin\Qt6Core.dll"
# → 应输出 True

# 2. CMake 在不在
& "C:\Qt\Tools\CMake_64\bin\cmake.exe" --version
# → 应输出 "cmake version 3.30.x"

# 3. jom 在不在
Test-Path "C:\Qt\Tools\QtCreator\bin\jom\jom.exe"
# → 应输出 True（路径可能略不同，自己 dir 找一下）

# 4. vcvars64 在不在
Test-Path "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
# → 应输出 True
```

任何一项 False，回 §1 装齐。

---

## 4. 改路径配置（重要）

老电脑的脚本 / 配置可能写死了 `D:\Qt\6.10.3\...` 这种路径，搬到 C 盘后要改。

### 4.1 找到所有写死的 Qt 路径

```powershell
cd C:\dev\oilPro
# 搜所有 "D:\Qt" 出现的地方
Select-String -Path .\* -Pattern "D:\\Qt" -Recurse -Exclude build,bin,.vs
```

理论上 CMakeLists.txt 里**不应该**有写死的路径（项目用的是 `find_package(Qt6)`，靠 CMAKE_PREFIX_PATH 找 Qt）。如果搜到了，把那处的路径改成新机器上的实际路径。

### 4.2 写一份本机的构建脚本

新建 `C:\Temp\build_oilpro.cmd`（**注意改成你新电脑的实际路径**）：

```bat
@echo off
setlocal

set QT_DIR=C:\Qt\6.10.3\msvc2022_64
set CMAKE=C:\Qt\Tools\CMake_64\bin\cmake.exe
set VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat
set SRC=C:\dev\oilPro
set BLD=%SRC%\build\Desktop_Qt_6_10_3_MSVC2022_64bit-Debug

call "%VCVARS%" >nul

if not exist "%BLD%" (
    "%CMAKE%" -S "%SRC%" -B "%BLD%" -G "NMake Makefiles JOM" ^
        -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
        -DCMAKE_BUILD_TYPE=Debug ^
        -DWITH_HDF5=OFF ^
        > "C:\Temp\bld.log" 2>&1
)

"%CMAKE%" --build "%BLD%" >> "C:\Temp\bld.log" 2>&1
set RC=%ERRORLEVEL%
echo EXITCODE=%RC%
exit /b %RC%
```

**`mkdir C:\Temp` 如果还没有的话先建一下。**

---

## 5. 首次构建（10 ~ 30 分钟）

```powershell
cmd /c C:\Temp\build_oilpro.cmd
```

预期输出最后一行：`EXITCODE=0`

构建产物（exe）：`C:\dev\oilPro\bin\windows\x64\DEBUG\StratumExplorer.exe`

### 5.1 看构建日志

```powershell
Get-Content C:\Temp\bld.log -Tail 50
```

### 5.2 常见首次构建错误

| 错误信息 | 原因 | 解决 |
|---|---|---|
| `fatal error C1083: 'type_traits'` | 没在 vcvars 里跑 | build_oilpro.cmd 已 `call vcvars64.bat`，再没用就检查 VS 是否装了 C++ 负载 |
| `Could not find Qt6 ...` | CMAKE_PREFIX_PATH 不对 | 修改 build_oilpro.cmd 里的 `QT_DIR` |
| `'jom' is not recognized` | jom 不在 PATH | vcvars 之后应该自动有；如果没有，在 build_oilpro.cmd 里加 `set PATH=C:\Qt\Tools\QtCreator\bin\jom;%PATH%` |
| `LNK2005 IWorkflowEngine 重复定义` | core_interfaces_service_processing 退化成 INTERFACE 库 | 检查 `src/core/interfaces/service/processing/CMakeLists.txt` 是不是 STATIC 库，moc_anchor.cpp 还在 |
| `Q_DECL_IMPORT` 相关 LNK | 误定义了 `QCUSTOMPLOT_USE_LIBRARY` | 检查 `third_party/qcustomplot/CMakeLists.txt`，**不要**有这个宏 |
| `Cannot open include file: 'hdf5.h'` | HDF5 没禁 | 确认脚本里 `-DWITH_HDF5=OFF` |
| 中文乱码 | 控制台是 GBK，不影响功能 | 忽略 |

---

## 6. 跑起来看效果

```powershell
cd C:\dev\oilPro\bin\windows\x64\DEBUG
.\StratumExplorer.exe
```

**第一次启动可能要等几秒**（Qt 加载样式 / 翻译）。

### 6.1 如果启动报 "找不到 Qt6Core.dll"

需要把 Qt 的 DLL 部署到 exe 旁边：

```powershell
& "C:\Qt\6.10.3\msvc2022_64\bin\windeployqt.exe" `
  "C:\dev\oilPro\bin\windows\x64\DEBUG\StratumExplorer.exe"
```

或者把 `C:\Qt\6.10.3\msvc2022_64\bin` 加到 PATH。

### 6.2 验证最近一轮（Phase D）功能

打开后应该看到：
- 主窗口 → "数据处理" Tab → 工作流编辑器
- 顶部工具栏有：新建 / 打开模板 / 打开工程 / 保存模板 / 保存工程 / **另存为...** ← Phase D 加的
- 左侧节点库 → 拖一个 demo 节点到画布 → **双击节点** → 弹 PlotDialog
- 编辑器**底部**有日志面板（LogDock，等级过滤 + 关键字过滤）

---

## 7. 如果你（AI）要继续往下做

按 `BACKEND_HANDOFF.md` §10 的顺序：
- A. 写 `ExternalProcessNode : IProcessNode`，run() 转给 `ExternalProcessRunner`
- B. 把后端给的节点登记单转成 `NodeMeta` 注册到 `NodeRegistry`
- C. 接入 HDF5（vcpkg manifest 模式，开 `WITH_HDF5=ON`）
- D. 跑通 input.las → preprocess.despike → preprocess.bandpass → display.line

**任何代码改动后，跑一次 `cmd /c C:\Temp\build_oilpro.cmd`，确认 EXITCODE=0 再继续。**

### 我的代码风格约定

- 头文件保护用 `#ifndef XXX_H` / `#define` / `#endif`，不用 `#pragma once`
- 命名空间：core 用 `processing::`，UI 用 `processing::gui::`
- Qt 信号槽永远用函数指针：`connect(s, &Class::signal, r, &Class::slot)`
- UI 头文件不许 include 进 core/
- 我新加的头文件统一加文件注释：
  ```cpp
  // =============================================================================
  //  XxxYyy.h
  //  作者：工程师 ly
  //
  //  这个文件是干啥的（一句话）
  //  - 关键设计点 1
  //  - 关键设计点 2
  // =============================================================================
  ```
  **不写日期**。

---

## 8. 万一 build 怎么都过不去

把 `C:\Temp\bld.log` **完整复制**贴给 AI，加一句 "帮我看构建日志"。
最近一次成功构建是 EXITCODE=0，文件清单见 `PROJECT_CONTEXT.md` §4.1。

---

文档结束。环境跑通了就去看 PROJECT_CONTEXT.md 和 BACKEND_HANDOFF.md。
