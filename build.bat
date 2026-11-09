@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

if "%QTDIR%"=="" set "QTDIR=D:\Qt6.11\6.11.2\mingw_64"
if "%MINGW_BIN%"=="" set "MINGW_BIN=D:\Qt6\Tools\mingw1310_64\bin"

if not exist "%QTDIR%\bin\qmake.exe" (
  echo [ERROR] qmake not found: "%QTDIR%\bin\qmake.exe"
  echo Set QTDIR to your Qt 6 mingw_64 directory.
  exit /b 1
)
if not exist "%MINGW_BIN%\mingw32-make.exe" (
  echo [ERROR] mingw32-make not found: "%MINGW_BIN%\mingw32-make.exe"
  echo Set MINGW_BIN to your MinGW bin directory.
  exit /b 1
)

set "PATH=%MINGW_BIN%;%QTDIR%\bin;%PATH%"
if not exist build mkdir build
pushd build

echo [1/2] qmake...
"%QTDIR%\bin\qmake.exe" ..\DeepDesignStudio.pro -spec win32-g++
if errorlevel 1 (
  popd
  echo [ERROR] qmake failed.
  exit /b 1
)

echo [2/2] build...
"%MINGW_BIN%\mingw32-make.exe" -j8
if errorlevel 1 (
  popd
  echo [ERROR] build failed.
  exit /b 1
)

popd
echo.
echo Build OK: %~dp0build\bin\DeepDesignStudio.exe
endlocal
