# oilPro 环境配置与接力指南 — MIGRATION_GUIDE

> **场景1**：换电脑 → 配置新环境
> **场景2**：AI 上下文满了 → 新窗口接力
> **作者**：工程师 ly
> **本指南配合**：`PROJECT_CONTEXT.md`（架构）+ `BACKEND_HANDOFF.md`（前后端约定）一起看。

---

## 0. 给新 AI 窗口的开场白（直接粘贴）

```text
你接手一个 Qt 6.10.3 + MSVC2022 的 C++ 桌面项目 oilPro。
项目路径：D:\pro\Demo\oilPro\

请按这个顺序读文档：
1. PROJECT_CONTEXT.md  — 整体架构、约定、节点清单
2. HANDOFF_CHECKLIST.md  — 当前进度、待办任务
3. BACKEND_HANDOFF.md  — 与后端 EXE 的 JSON 配置协议

快速构建：
  cmd /c D:\pro\Demo\oilPro\build_oilpro.cmd

最近一次构建：EXITCODE=0（2026-05-18，v1.0）
已注册 42 个节点；属性面板参数见 node_client_params.json
当前协议：JSON 任务配置文件调 EXE（业务 HDF5 由后端读写）
下一版本：展示界面开发（PlotDialog / Display 节点 UI）
```

---

## 0.1 发版后同步 GitHub

仓库：https://github.com/liyue-2021/StratumExplorer  

```powershell
git push origin master
```

**每次 push 前**先更新记录文档（详见 `HANDOFF_CHECKLIST.md` §0.2）：

- `HANDOFF_CHECKLIST.md`（必改）
- `PROJECT_CONTEXT.md`
- `MIGRATION_GUIDE.md`（含本文件 §0 开场白）
- `BACKEND_HANDOFF.md` / `README.md`（有变更才改）

---

## 1. 环境要求

| 软件 | 版本 | 路径 |
|------|------|------|
| Qt | 6.10.3 MSVC2022 64-bit | `C:\Qt\6.10.3\msvc2022_64` |
| CMake | 3.21~3.30（**不要用 4.x**） | `C:\Qt\Tools\CMake_64\bin\cmake.exe` |
| VS 2022 | 含 C++ 桌面开发 | `C:\Program Files\Microsoft Visual Studio\2022\Community` |

### 验证环境

```powershell
Test-Path "C:\Qt\6.10.3\msvc2022_64\bin\Qt6Core.dll"  # 应输出 True
& "C:\Qt\Tools\CMake_64\bin\cmake.exe" --version       # 应输出 3.30.x
Test-Path "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"  # 应输出 True
```

---

## 2. 构建脚本

项目已有构建脚本：`D:\pro\Demo\oilPro\build_oilpro.cmd`

```powershell
# 构建
cmd /c D:\pro\Demo\oilPro\build_oilpro.cmd

# 查看日志
Get-Content D:\pro\Demo\oilPro\build_oilpro.log -Tail 30
```

预期输出最后一行：`EXITCODE=0`

---

## 3. 常见构建错误

| 错误 | 解决 |
|------|------|
| `type_traits` 找不到 | 先 `vcvars64.bat` |
| `Could not find Qt6` | 修改 `build_oilpro.cmd` 里的 `QT_DIR` |
| `LNK2005` | 确认 `moc_anchor.cpp` 存在 |
| `QCUSTOMPLOT_USE_LIBRARY` | **不要**定义这个宏 |
| 中文乱码 | **不是 bug**，忽略 |

---

## 4. 代码风格约定

- 头文件保护：`#ifndef XXX_H` / `#define` / `#endif`（不用 `#pragma once`）
- 命名空间：core 用 `processing::`，UI 用 `processing::gui::`
- Qt 信号槽：`connect(s, &Class::signal, r, &Class::slot)` 函数指针风格
- UI 头文件**不许** include 进 core/

---

文档结束。
