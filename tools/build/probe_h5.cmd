@echo off
setlocal
rem Usage: probe_h5.cmd path\to\file.h5
set "EXE=%~dp0..\..\build\Desktop_Qt_6_10_3_MSVC2022_64bit-Debug-HDF5\src\app\StratumExplorer.exe"
if not exist "%EXE%" (
    echo Build HDF5 first: tools\build\build_oilpro_hdf5.cmd
    exit /b 1
)
if "%~1"=="" (
    echo Usage: probe_h5.cmd file.h5
    exit /b 1
)
rem TODO: add --probe-h5 CLI; for now document H5PlotReader in core
echo HDF5 build at: %EXE%
echo Pass file: %~1
exit /b 0
