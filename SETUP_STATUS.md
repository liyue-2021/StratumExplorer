# oilPro 开发环境配置状态报告

**生成时间**：2026-04-29
**项目路径**：d:\pro\Demo\oilPro

---

## 📊 当前环境检查结果

| 工具 | 版本 | 状态 | 位置 |
|------|------|------|------|
| **CMake** | 3.30.5 ✅ | **已安装** | 系统 PATH |
| **Visual Studio 2022** | - | **❌ 未安装或不完整** | - |
| **Qt 6.10.3** | - | **❌ 未安装** | - |
| **vcpkg** | - | ⚠️ 可选（暂不需要） | - |

---

## 🎯 立即需要做的事

### 1️⃣ **安装 Visual Studio 2022 Community**（必须）

**下载链接**：https://visualstudio.microsoft.com/zh-hans/downloads/

**安装步骤**：
1. 选择"Visual Studio Community 2022"
2. 点击"下载"运行安装器
3. 选择工作负载：
   - ✅ **使用 C++ 的桌面开发**
4. 在"可选"中勾选：
   - ✅ MSVC v143 compiler
   - ✅ Windows 11 SDK
   - ✅ CMake 工具（可选，已有 cmake 3.30）
5. 完成安装（会下载 ~10GB，耗时 15-30 分钟）

**验证**：安装后运行命令检查
```powershell
# 应该看到 VS 2022 版本（如 Community / Professional / Enterprise）
Get-ChildItem "C:\Program Files (x86)\Microsoft Visual Studio\2022"
```

---

### 2️⃣ **安装 Qt 6.10.3**（必须）

**下载链接**：https://www.qt.io/download-qt-installer

**前置**：需要注册免费 Qt 账号（开源许可证）

**安装步骤**：
1. 运行 Qt 在线安装器
2. 登录 Qt 账号
3. 选择"自定义安装"
4. 勾选组件：
   - ✅ **Qt** → **6.10.0** 或 **6.10.3**
   - ✅ **MSVC 2022 64-bit**
   - ✅ Qt Debug Information Files
   - ✅ Developer and Designer Tools → **CMake** (optional)
   - ✅ Developer and Designer Tools → **Ninja** (optional)
   - ✅ Developer and Designer Tools → **Qt Creator** (optional, 用于 UI 编辑)

5. **重要**：安装路径改为 `D:\Qt\6.10.3\msvc2022_64`（或记住实际安装路径）
6. 完成安装（~5GB，耗时 5-15 分钟）

**验证**：安装后运行
```powershell
Test-Path "D:\Qt\6.10.3\msvc2022_64\bin\qmake.exe"  # 应返回 True
```

---

### 3️⃣ **配置 VS Code 环境**（安装完 Qt 后做）

#### a) 打开项目
```powershell
cd d:\pro\Demo\oilPro
code .
```

#### b) 安装 VS Code 扩展
按 `Ctrl+Shift+X`，搜索并安装：
- **C/C++** (Microsoft) — C++ 代码补全、调试
- **CMake Tools** (Microsoft) — CMake 集成，自动构建
- **CMake** (twxs) — CMakeLists.txt 语法高亮
- **Qt All Extensions Pack** (The Qt Company) — Qt .ui 文件支持

#### c) 创建 `.vscode/settings.json`
在项目根目录创建 `.vscode/settings.json`，内容如下（**修改 Qt 路径为实际安装路径**）：

```json
{
  "cmake.configureSettings": {
    "CMAKE_PREFIX_PATH": "D:\\Qt\\6.10.3\\msvc2022_64",
    "Qt6_DIR": "D:\\Qt\\6.10.3\\msvc2022_64\\lib\\cmake\\Qt6",
    "WITH_HDF5": "OFF"
  },
  "cmake.buildDirectory": "${workspaceFolder}/build",
  "cmake.generator": "Ninja Multi-Config",
  "cmake.preferredGenerators": ["Ninja Multi-Config"]
}
```

#### d) 配置 CMake Kit
1. 按 `Ctrl+Shift+P`，输入 `CMake: Scan for Kits`，回车
2. 再按 `Ctrl+Shift+P`，输入 `CMake: Select a Kit`，选择：
   - **Visual Studio Community 2022 Release - amd64**（或类似的 MSVC 2022 选项）

#### e) 配置项目
按 `Ctrl+Shift+P`，输入 `CMake: Configure`

等待配置完成（首次会下载依赖，可能需要 2-5 分钟）。如果看到这样的输出说明成功：
```
[cmake] Build files have been written to: <path>/build
```

#### f) 构建项目
按 `Ctrl+Shift+P`，输入 `CMake: Build`

等待编译完成。成功的标志是看到：
```
[build] Built target app
```

---

## 📋 完整的安装时间表

| 步骤 | 时间 | 备注 |
|------|------|------|
| 1. 下载 VS 2022 安装器 | 5-10 分钟 | 文件 ~2GB |
| 2. 安装 VS 2022 | 30-60 分钟 | 取决于网速和磁盘速度 |
| 3. 下载 Qt 在线安装器 | 5 分钟 | 文件 ~500MB |
| 4. 注册 Qt 账号 | 3 分钟 | 免费开源许可证 |
| 5. 安装 Qt 6.10 | 15-30 分钟 | 文件 ~5GB |
| 6. 安装 VS Code 扩展 | 2 分钟 | 自动下载 |
| 7. 配置 VS Code | 3 分钟 | 编辑 JSON 文件 |
| 8. CMake Configure | 2-5 分钟 | 首次会比较慢 |
| 9. CMake Build | 5-15 分钟 | 取决于CPU |
| **总计** | **80-150 分钟** | **约 1.5-2.5 小时** |

---

## ⚠️ 常见问题

### Q：安装器下载到一半，能不能断点续传？
**A**：可以，只要不清空缓存就能继续。建议在网络稳定的时段下载。

### Q：Qt 能装到其他路径吗？
**A**：可以，但要记住实际路径，并在 `.vscode/settings.json` 的 `CMAKE_PREFIX_PATH` 中更新。

### Q：第一次编译失败，怎么办？
**A**：
1. 检查错误信息（通常能看到具体缺少什么）
2. 常见原因：Qt 路径错误、MSVC 没装完整、缺 Windows SDK
3. 删除 `build` 目录，重新 `CMake: Configure`

### Q：能用 Qt Creator 替代 VS Code 编译吗？
**A**：可以，Qt Creator 内置 Qt 和 CMake，但这个项目建议用 VS Code + CMake Tools。

### Q：要装 vcpkg 和 HDF5 吗？
**A**：目前不需要，`WITH_HDF5=OFF` 已禁用。后面真正接入后端算法时再装。

---

## ✅ 下一步检查清单

完成上述安装后，逐一验证：

- [ ] VS 2022 Community 已安装，可以找到 `vcvars64.bat`
- [ ] Qt 6.10.3 已安装，`D:\Qt\6.10.3\msvc2022_64\bin\qmake.exe` 存在
- [ ] VS Code 打开项目，能看到 `CMakeLists.txt` 没有红波浪
- [ ] `CMake: Select a Kit` 能选中 MSVC 2022
- [ ] `CMake: Configure` 完成，无大量错误
- [ ] `CMake: Build` 成功，看到 `Built target app`
- [ ] 在 `build/bin/windows/x64/Debug/` 看到 `app.exe`

完成所有检查后，可以回来继续开发！

---

**需要帮助？** 把错误信息粘贴给我，我来帮你排查。
