@echo off
call "%~dp0build.bat" %1 || exit /b 1
"%~dp0build\bin\Kartovasiya.exe"
