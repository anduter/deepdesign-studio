@echo off
setlocal
cd /d "%~dp0"

if "%QTDIR%"=="" set "QTDIR=D:\Qt6.11\6.11.2\mingw_64"
if "%MINGW_BIN%"=="" set "MINGW_BIN=D:\Qt6\Tools\mingw1310_64\bin"
set "PATH=%MINGW_BIN%;%QTDIR%\bin;%PATH%"

call build.bat
if errorlevel 1 exit /b 1

set "DIST=%~dp0dist\DeepDesignStudio"
if exist "%DIST%" rmdir /s /q "%DIST%"
mkdir "%DIST%"

copy /y "build\bin\DeepDesignStudio.exe" "%DIST%\" >nul
"%QTDIR%\bin\windeployqt.exe" --release --no-translations --no-system-d3d-compiler --no-opengl-sw "%DIST%\DeepDesignStudio.exe"
if errorlevel 1 exit /b 1

xcopy /e /i /y "docs" "%DIST%\docs" >nul
xcopy /e /i /y "examples" "%DIST%\examples" >nul
xcopy /e /i /y "screenshots" "%DIST%\screenshots" >nul
copy /y "README.md" "%DIST%\" >nul
copy /y "LICENSE" "%DIST%\" >nul
copy /y "THIRD_PARTY_NOTICES.md" "%DIST%\" >nul
copy /y "CONTRIBUTING.md" "%DIST%\" >nul
(
  echo @echo off
  echo cd /d "%%~dp0"
  echo start "" "%%~dp0DeepDesignStudio.exe" %%*
) > "%DIST%\run.bat"

echo.
echo Deployed: %DIST%\DeepDesignStudio.exe
endlocal
