@echo off
setlocal enabledelayedexpansion

rem Debug build WITH HDF5 + zlib (first configure downloads zlib/hdf5, may take 10+ min)
set "QT_DIR=C:\Qt\6.10.3\msvc2022_64"
set "CMAKE=C:\Qt\Tools\CMake_64\bin\cmake.exe"
set "VCVARS=C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat"

set "SRC=%~dp0..\.."
set "BLD=%SRC%\build\Desktop_Qt_6_10_3_MSVC2022_64bit-Debug-HDF5"
set "LOG=%~dp0logs\build_oilpro_hdf5.log"

if not exist "%~dp0logs" mkdir "%~dp0logs"

echo ===============================================
echo StratumExplorer Debug + HDF5/zlib
echo Build dir: %BLD%
echo Log: %LOG%
echo ===============================================

if not exist "%BLD%" mkdir "%BLD%"

if not exist "%VCVARS%" (
    echo ERROR: vcvars64.bat not found: %VCVARS%
    exit /b 1
)

call "%VCVARS%" >nul 2>&1

if not exist "%BLD%\CMakeCache.txt" (
    echo Configuring WITH_HDF5=ON ...
    call "%CMAKE%" -S "%SRC%" -B "%BLD%" -G "NMake Makefiles" ^
        -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
        -DCMAKE_BUILD_TYPE=Debug ^
        -DWITH_HDF5=ON > "%LOG%" 2>&1
    if errorlevel 1 (
        echo Configure failed. See %LOG%
        exit /b 1
    )
)

echo Building...
call "%CMAKE%" --build "%BLD%" >> "%LOG%" 2>&1
if errorlevel 1 (
    echo Build failed. See %LOG%
    exit /b 1
)

echo Build succeeded. Open this build dir in Qt Creator or copy StratumExplorer.exe from:
echo   %BLD%
exit /b 0
