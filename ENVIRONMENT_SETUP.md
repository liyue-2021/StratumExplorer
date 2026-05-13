# oilPro Qt 开发环境配置指南

> **当前状态**（2026-04-29）：
> - ✅ CMake 3.30.5 已安装
> - ❌ VS 2022 需要完整安装（目前不完整）
> - ❌ Qt 6.10.3 需要安装
> - ❌ vcpkg 需要安装（用于 HDF5 等三方库）

---

## 第一步：安装/修复 Visual Studio 2022

### 检查现状
```powershell
# 查看是否安装了VS 2022
Get-ChildItem "C:\Program Files (x86)\Microsoft Visual Studio\2022" -ErrorAction SilentlyContinue
```

### 安装方案

**选项 A：首次安装**
1. 下载 VS 2022 Community：https://visualstudio.microsoft.com/zh-hans/downloads/
2. 运行安装器
3. **必选工作负载**：
   - ✅ 使用 C++ 的桌面开发
4. **可选组件**（安装器中选）：
   - ✅ MSVC v143 - VS 2022 C++ x64/x86 生成工具
   - ✅ Windows 11 SDK（或最新版）
   - ✅ 适用于 Windows 的 CMake 工具
   - ✅ Git for Windows

**选项 B：已装但不完整**
1. 找到安装器，点击"修改"
2. 确保上面的工作负载和组件都勾选了
3. 完成安装

**验证安装**
```powershell
# 安装后，应该能找到 vcvars64.bat
Test-Path "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

---

## 第二步：安装 Qt 6.10.3

### 下载
https://www.qt.io/download-qt-installer

### 注册账号
需要注册免费的Qt账号（开源许可证）

### 安装步骤
1. 运行安装器
2. 登录 Qt 账号
3. 选择**自定义安装**
4. 选择组件：
   - ✅ **Qt 6.10.0** → **MSVC 2022 64-bit**（注意：6.10.3 应该是最新的 6.10）
   - ✅ **Qt 6.10.0** → Qt Debug Information Files
   - ✅ Developer and Designer Tools → **CMake** v3.24+
   - ✅ Developer and Designer Tools → **Ninja**
   - ✅ Developer and Designer Tools → **Qt Creator**（可选，用于编辑 .ui 文件）

5. 安装路径建议改为：`D:\Qt\6.10.3` 或 `D:\Qt\6.10.3\msvc2022_64`

### 验证安装
```powershell
# 安装后应该能找到这个目录
Test-Path "D:\Qt\6.10.3\msvc2022_64\bin\qmake.exe"
Test-Path "D:\Qt\6.10.3\msvc2022_64\bin\cmake.exe"
```

---

## 第三步：配置环境变量（可选但推荐）

设置系统环境变量，这样命令行可以直接用：

```powershell
# 以管理员身份打开 PowerShell，然后运行：
$QtPath = "D:\Qt\6.10.3\msvc2022_64\bin"
$CMakePath = "D:\Qt\6.10.3\msvc2022_64\bin"

# 添加到 PATH
[Environment]::SetEnvironmentVariable(
    "PATH",
    [Environment]::GetEnvironmentVariable("PATH", "User") + ";$QtPath;$CMakePath",
    "User"
)

Write-Host "Environment variables updated. Restart PowerShell for changes to take effect."
```

---

## 第四步：安装 vcpkg（处理 HDF5）

项目目前用 `WITH_HDF5=OFF` 禁用了 HDF5，但以后会用到。现在可以先装好 vcpkg：

```powershell
# 以管理员身份打开 PowerShell
cd C:\
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# 设置 VCPKG_ROOT 环境变量
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\vcpkg", "User")

# （可选）预装 hdf5，但目前不需要
# C:\vcpkg\vcpkg install hdf5:x64-windows
```

---

## 第五步：在 VS Code 中打开项目并编译

### 1. 打开项目
```powershell
cd d:\pro\Demo\oilPro
code .
```

### 2. VS Code 扩展

安装这些扩展（按 `Ctrl+Shift+X`）：
- **C/C++**（Microsoft）
- **CMake Tools**（Microsoft）
- **CMake**（twxs）
- **Qt All Extensions Pack**（The Qt Company）

### 3. 配置 CMake Kit

按 `Ctrl+Shift+P`，输入：
```
CMake: Scan for Kits
```

然后再输入：
```
CMake: Select a Kit
```

选择 **Visual Studio Community 2022 Release - amd64**（或类似的选项）

### 4. 配置项目

编辑 `.vscode/settings.json`，添加：

```json
{
  "cmake.configureSettings": {
    "CMAKE_PREFIX_PATH": "D:\\Qt\\6.10.3\\msvc2022_64",
    "Qt6_DIR": "D:\\Qt\\6.10.3\\msvc2022_64\\lib\\cmake\\Qt6",
    "VCPKG_ROOT": "C:\\vcpkg",
    "WITH_HDF5": "OFF"
  },
  "cmake.buildDirectory": "${workspaceFolder}/build",
  "cmake.generator": "Ninja Multi-Config"
}
```

### 5. 编译项目

按 `Ctrl+Shift+P`，输入：
```
CMake: Configure
```

等待配置完成（首次会比较慢），然后：
```
CMake: Build
```

### 6. 运行程序

编译完成后，在 `.vscode/launch.json` 中配置调试：

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug oilPro",
      "type": "cppdbg",
      "request": "launch",
      "program": "${workspaceFolder}/build/src/app/Debug/app.exe",
      "args": [],
      "stopAtEntry": false,
      "cwd": "${workspaceFolder}",
      "environment": [
        {
          "name": "PATH",
          "value": "D:\\Qt\\6.10.3\\msvc2022_64\\bin;${env:PATH}"
        }
      ],
      "externalConsole": false,
      "MIMode": "gdb",
      "setupCommands": [
        {
          "description": "Enable pretty-printing for gdb",
          "text": "-enable-pretty-printing",
          "ignoreFailures": true
        }
      ]
    }
  ]
}
```

---

## 故障排查

### 问题 1：`CMake: Configure` 找不到 Qt
**解决**：
- 确保 `CMAKE_PREFIX_PATH` 指向正确的 Qt 路径
- 重新扫描 Kits：`CMake: Scan for Kits`
- 重启 VS Code

### 问题 2：编译错误 `cannot find include file: 'type_traits'`
**解决**：
- 这是因为没有在 MSVC 环境下编译
- 用 `C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat` 激活环境
- 或者直接用 Qt Creator 编译（它会自动处理）

### 问题 3：运行时找不到 Qt DLL
**解决**：
- 把 `D:\Qt\6.10.3\msvc2022_64\bin` 加到 PATH 环境变量
- 或者在 launch.json 的 `environment` 中加入

### 问题 4：`qmake: command not found`
**解决**：
- 确保 Qt 已安装到 `D:\Qt\6.10.3\msvc2022_64`
- 检查 `D:\Qt\6.10.3\msvc2022_64\bin\qmake.exe` 是否存在

---

## 快速检查清单

在开始编译前，逐一检查：

```powershell
# 1. CMake 版本 >= 3.16
cmake --version

# 2. MSVC 编译器可用（激活后）
# 在 vcvars64.bat 后运行：
cl.exe

# 3. Qt 路径正确
Test-Path "D:\Qt\6.10.3\msvc2022_64\bin\qmake.exe"
Test-Path "D:\Qt\6.10.3\msvc2022_64\lib\cmake\Qt6\Qt6Config.cmake"

# 4. vcpkg 初始化（可选）
Test-Path "C:\vcpkg\vcpkg.exe"
```

---

## 下一步

完成上述安装后，回到 oilPro 根目录运行：

```powershell
cd d:\pro\Demo\oilPro

# 激活 MSVC 环境
$vc = "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cmd /c "`"$vc`" && cmake --version"

# 创建 build 目录
mkdir -Force build | Out-Null
cd build

# 配置并编译
cmake -G "Ninja Multi-Config" `
  -DCMAKE_PREFIX_PATH="D:\Qt\6.10.3\msvc2022_64" `
  -DWITH_HDF5=OFF `
  ..

ninja
```

或者直接在 VS Code 中按照第五步操作，更方便。

---

**文档更新于**：2026-04-29
**维护者**：GitHub Copilot + 用户
