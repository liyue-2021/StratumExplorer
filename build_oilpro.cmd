@echo off
setlocal enabledelayedexpansion

rem oilPro build script for current machine
rem Adjust paths below if your installation differs.

set "QT_DIR=C:\Qt\6.10.3\msvc2022_64"
set "CMAKE=C:\Qt\Tools\CMake_64\bin\cmake.exe"
set "VCVARS=C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat"
set "SRC=%~dp0"
set "BLD=%SRC%build\Desktop_Qt_6_10_3_MSVC2022_64bit-Debug"
set "LOG=%SRC%build_oilpro.log"

echo ===============================================
echo oilPro build script
echo Source: %SRC%
echo Build dir: %BLD%
echo Qt dir: %QT_DIR%
echo CMake: %CMAKE%
echo VS vars: %VCVARS%
echo Log: %LOG%
echo ===============================================

if not exist "%BLD%" mkdir "%BLD%"

if not exist "%VCVARS%" (
    echo ERROR: Visual Studio vcvars64.bat not found: %VCVARS%
    exit /b 1
)

if not exist "%CMAKE%" (
    echo ERROR: Qt CMake not found: %CMAKE%
    exit /b 1
)

rem Configure if needed
if not exist "%BLD%\CMakeCache.txt" (
    echo Configuring project...
    call "%VCVARS%" >nul 2>&1
    call "%CMAKE%" -S "%SRC%" -B "%BLD%" -G "NMake Makefiles" -DCMAKE_PREFIX_PATH="%QT_DIR%" -DCMAKE_BUILD_TYPE=Debug -DWITH_HDF5=OFF > "%LOG%" 2>&1
    if errorlevel 1 (
        echo Configure failed. See %LOG%
        exit /b 1
    )
)

echo Building project...
call "%VCVARS%" >nul 2>&1
call "%CMAKE%" --build "%BLD%" > "%LOG%" 2>&1
if errorlevel 1 (
    echo Build failed. See %LOG%
    exit /b 1
)

echo Build succeeded.
exit /b 0
