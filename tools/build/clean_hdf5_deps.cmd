@echo off
setlocal
rem Run when HDF5 configure hangs at "Fetching HDF5" (stuck git clone).
set "BLD=%~dp0..\..\build\Desktop_Qt_6_10_3_MSVC2022_64bit-Debug-HDF5"
echo Stopping stray cmake/nmake (ignore errors)...
taskkill /F /IM cmake.exe /T 2>nul
taskkill /F /IM nmake.exe /T 2>nul
echo Removing incomplete FetchContent deps in:
echo   %BLD%\_deps
if exist "%BLD%\_deps\hdf5-src" rmdir /s /q "%BLD%\_deps\hdf5-src"
if exist "%BLD%\_deps\hdf5-build" rmdir /s /q "%BLD%\_deps\hdf5-build"
if exist "%BLD%\_deps\hdf5-subbuild" rmdir /s /q "%BLD%\_deps\hdf5-subbuild"
if exist "%BLD%\CMakeCache.txt" del /q "%BLD%\CMakeCache.txt"
if exist "%BLD%\CMakeFiles" rmdir /s /q "%BLD%\CMakeFiles"
echo Done. Re-run: tools\build\build_oilpro_hdf5.cmd
exit /b 0
