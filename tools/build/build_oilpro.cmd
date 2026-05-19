@echo off
setlocal enabledelayedexpansion

rem Debug build — adjust Qt/VS paths for your machine.
set "QT_DIR=C:\Qt\6.10.3\msvc2022_64"
set "CMAKE=C:\Qt\Tools\CMake_64\bin\cmake.exe"
set "VCVARS=C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat"

rem Repo root (two levels up from tools\build\)
set "SRC=%~dp0..\.."
set "BLD=%SRC%\build\Desktop_Qt_6_10_3_MSVC2022_64bit-Debug"
set "LOG=%~dp0logs\build_oilpro.log"

if not exist "%~dp0logs" mkdir "%~dp0logs"

echo ===============================================
echo StratumExplorer Debug build
echo Source: %SRC%
echo Build dir: %BLD%
echo Log: %LOG%
echo ===============================================

if not exist "%BLD%" mkdir "%BLD%"

if not exist "%VCVARS%" (
    echo ERROR: vcvars64.bat not found: %VCVARS%
    exit /b 1
)

if not exist "%CMAKE%" (
    echo ERROR: CMake not found: %CMAKE%
    exit /b 1
)

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
