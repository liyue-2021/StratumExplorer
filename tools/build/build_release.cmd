@echo off
setlocal enabledelayedexpansion

set "QT_DIR=C:\Qt\6.10.3\msvc2022_64"
set "CMAKE=C:\Qt\Tools\CMake_64\bin\cmake.exe"
set "VCVARS=C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat"

set "SRC=%~dp0..\.."
set "BLD=%SRC%\build\Desktop_Qt_6_10_3_MSVC2022_64bit-Release"
set "LOG=%~dp0logs\build_release.log"

if not exist "%~dp0logs" mkdir "%~dp0logs"

echo Building Release in %BLD%
if not exist "%BLD%" mkdir "%BLD%"
call "%VCVARS%" >nul 2>&1
"%CMAKE%" -S "%SRC%" -B "%BLD%" -G "Ninja" -DCMAKE_PREFIX_PATH="%QT_DIR%" -DCMAKE_BUILD_TYPE=Release -DWITH_HDF5=OFF > "%LOG%" 2>&1
if errorlevel 1 (
    echo Configure failed. See %LOG%
    exit /b 1
)
"%CMAKE%" --build "%BLD%" --config Release --target app >> "%LOG%" 2>&1
if errorlevel 1 (
    echo Build failed. See %LOG%
    exit /b 1
)
echo Build done. Log: %LOG%
exit /b 0
