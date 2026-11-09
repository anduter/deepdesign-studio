@echo off
setlocal
cd /d "%~dp0"

if "%QTDIR%"=="" set "QTDIR=D:\Qt6.11\6.11.2\mingw_64"
if "%MINGW_BIN%"=="" set "MINGW_BIN=D:\Qt6\Tools\mingw1310_64\bin"
set "PATH=%MINGW_BIN%;%QTDIR%\bin;%PATH%"

if not exist "build\bin\DeepDesignStudio.exe" (
  call build.bat
  if errorlevel 1 exit /b 1
)

start "" "%~dp0build\bin\DeepDesignStudio.exe" %*
endlocal
