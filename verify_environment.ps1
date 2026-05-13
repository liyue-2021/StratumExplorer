#!/usr/bin/env powershell
# oilPro 环境自检和编译验证脚本
# 运行方式：右键→以管理员身份运行 PowerShell，然后：
#   Set-ExecutionPolicy -ExecutionPolicy Bypass -Scope Process
#   .\verify_environment.ps1

param(
    [switch]$SkipClean = $false,
    [switch]$Verbose = $false
)

function Write-Header {
    param([string]$Text)
    Write-Host "`n" + ("=" * 60) -ForegroundColor Cyan
    Write-Host $Text -ForegroundColor Cyan
    Write-Host ("=" * 60) -ForegroundColor Cyan
}

function Write-Status {
    param([string]$Message, [string]$Status)
    if ($Status -eq "OK") {
        Write-Host "✅ $Message" -ForegroundColor Green
    } elseif ($Status -eq "WARN") {
        Write-Host "⚠️  $Message" -ForegroundColor Yellow
    } else {
        Write-Host "❌ $Message" -ForegroundColor Red
    }
}

function Test-Command {
    param([string]$Command)
    try {
        & $Command --version 2>&1 | Select-Object -First 1
        return $true
    } catch {
        return $false
    }
}

Write-Header "📋 oilPro 环境检查"

# ============= 检查 CMake =============
Write-Host "`n[1/6] 检查 CMake..." -ForegroundColor White
if (Test-Command cmake) {
    $cmakeVer = (cmake --version | Select-Object -First 1)
    Write-Status "CMake: $cmakeVer" "OK"
} else {
    Write-Status "CMake 未安装或不在 PATH 中" "FAIL"
}

# ============= 检查 VS 2022 =============
Write-Host "`n[2/6] 检查 Visual Studio 2022..." -ForegroundColor White
$vs2022Path = "C:\Program Files (x86)\Microsoft Visual Studio\2022"
if (Test-Path $vs2022Path) {
    $editions = Get-ChildItem $vs2022Path | Select-Object -ExpandProperty Name
    Write-Status "VS 2022 版本: $($editions -join ', ')" "OK"

    foreach ($edition in $editions) {
        $vcvarsPath = "$vs2022Path\$edition\VC\Auxiliary\Build\vcvars64.bat"
        if (Test-Path $vcvarsPath) {
            Write-Status "vcvars64.bat 位置: $vcvarsPath" "OK"
        }
    }
} else {
    Write-Status "未找到 VS 2022。请访问 https://visualstudio.microsoft.com/zh-hans/downloads/" "FAIL"
}

# ============= 检查 Qt =============
Write-Host "`n[3/6] 检查 Qt 6.10.3..." -ForegroundColor White
$qtPaths = @(
    "D:\Qt\6.10.3\msvc2022_64",
    "C:\Qt\6.10.3\msvc2022_64",
    "D:\Qt\6.10\msvc2022_64",
    "C:\Qt\6.10\msvc2022_64"
)

$qtFound = $false
foreach ($qtPath in $qtPaths) {
    if (Test-Path "$qtPath\bin\qmake.exe") {
        Write-Status "Qt 位置: $qtPath" "OK"
        $qtFound = $true
        $Script:QtPath = $qtPath
        break
    }
}

if (-not $qtFound) {
    Write-Status "未找到 Qt 6.10。请从 https://www.qt.io/download-qt-installer 下载安装" "FAIL"
    Write-Host "`n推荐安装路径: D:\Qt\6.10.3\msvc2022_64`n" -ForegroundColor Yellow
}

# ============= 检查 vcpkg =============
Write-Host "`n[4/6] 检查 vcpkg..." -ForegroundColor White
if (Test-Path "C:\vcpkg\vcpkg.exe") {
    Write-Status "vcpkg 已安装: C:\vcpkg" "OK"
} else {
    Write-Status "vcpkg 未安装（暂不需要，因为 WITH_HDF5=OFF）" "WARN"
}

# ============= 检查 project CMakeLists.txt =============
Write-Host "`n[5/6] 检查项目结构..." -ForegroundColor White
$projectRoot = "d:\pro\Demo\oilPro"
if (Test-Path "$projectRoot\CMakeLists.txt") {
    Write-Status "根 CMakeLists.txt 存在" "OK"
} else {
    Write-Status "项目根目录的 CMakeLists.txt 不存在" "FAIL"
}

if (Test-Path "$projectRoot\src\CMakeLists.txt") {
    Write-Status "src/CMakeLists.txt 存在" "OK"
} else {
    Write-Status "src/CMakeLists.txt 不存在" "FAIL"
}

# ============= 建议后续操作 =============
Write-Header "🔧 后续操作"

Write-Host "`n1️⃣  如果 Qt 未安装，请：" -ForegroundColor Yellow
Write-Host "   • 从 https://www.qt.io/download-qt-installer 下载 Qt 在线安装器" -ForegroundColor White
Write-Host "   • 安装 Qt 6.10.0+ (MSVC 2022 64-bit)" -ForegroundColor White
Write-Host "   • 建议安装路径：D:\Qt\6.10.3\msvc2022_64" -ForegroundColor White

Write-Host "`n2️⃣  安装完 Qt 后，在 VS Code 中：" -ForegroundColor Yellow
Write-Host "   • 按 Ctrl+Shift+X 安装扩展：" -ForegroundColor White
Write-Host "     - C/C++ (Microsoft)" -ForegroundColor White
Write-Host "     - CMake Tools (Microsoft)" -ForegroundColor White
Write-Host "     - CMake (twxs)" -ForegroundColor White
Write-Host "     - Qt All Extensions Pack (The Qt Company)" -ForegroundColor White

Write-Host "`n3️⃣  打开项目并配置：" -ForegroundColor Yellow
Write-Host "   • 命令行: cd d:\pro\Demo\oilPro ; code ." -ForegroundColor White
Write-Host "   • Ctrl+Shift+P → CMake: Scan for Kits" -ForegroundColor White
Write-Host "   • Ctrl+Shift+P → CMake: Select a Kit → 选择 Visual Studio Community 2022 Release - amd64" -ForegroundColor White

Write-Host "`n4️⃣  编辑 .vscode/settings.json：" -ForegroundColor Yellow
Write-Host "   • 设置 CMAKE_PREFIX_PATH 指向 Qt 安装路径" -ForegroundColor White

Write-Host "`n5️⃣  编译项目：" -ForegroundColor Yellow
Write-Host "   • Ctrl+Shift+P → CMake: Configure" -ForegroundColor White
Write-Host "   • Ctrl+Shift+P → CMake: Build" -ForegroundColor White

Write-Header "✅ 检查完成"
