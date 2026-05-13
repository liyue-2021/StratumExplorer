@echo off
setlocal enabledelayedexpansion
set "QT_DIR=C:\Qt\6.10.3\msvc2022_64"
set "CMAKE=C:\Qt\Tools\CMake_64\bin\cmake.exe"
set "VCVARS=C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat"
set "SRC=%~dp0"
set "SRC_NO_SLASH=%SRC:~0,-1%"
set "BLD=%SRC_NO_SLASH%\build\Desktop_Qt_6_10_3_MSVC2022_64bit-Release"

echo Building Release in %BLD%
if not exist "%BLD%" mkdir "%BLD%"
call "%VCVARS%" >nul 2>&1
"%CMAKE%" -S "%SRC_NO_SLASH%" -B "%BLD%" -G "Ninja" -DCMAKE_PREFIX_PATH="%QT_DIR%" -DCMAKE_BUILD_TYPE=Release -DWITH_HDF5=OFF
if errorlevel 1 (
    echo Configure failed
    exit /b 1
)
"%CMAKE%" --build "%BLD%" --config Release --target app
if errorlevel 1 (
    echo Build failed
    exit /b 1
)
echo Build done
