@echo off
REM ── Kartovasiya build script (Windows / MSVC) ──────────────────────────
setlocal
set ROOT=%~dp0
set CFG=%1
if "%CFG%"=="" set CFG=Release

for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set VS=%%i
if "%VS%"=="" ( echo [!] Visual Studio C++ tools not found. & exit /b 1 )

set CMAKE=%VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe
if not exist "%CMAKE%" set CMAKE=cmake

call "%VS%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 ( echo [!] vcvars64 failed & exit /b 1 )

"%CMAKE%" -S "%ROOT%." -B "%ROOT%build" -G Ninja -DCMAKE_BUILD_TYPE=%CFG% || exit /b 1
"%CMAKE%" --build "%ROOT%build" || exit /b 1

echo.
echo [ok] %ROOT%build\bin\Kartovasiya.exe
endlocal
