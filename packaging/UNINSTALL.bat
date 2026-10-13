@echo off
if not exist "%~dp0install.ps1" (
  echo Run UNINSTALL.bat from the extracted ZIP folder.
  pause
  exit /b 1
)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0install.ps1" -Uninstall %*
set "RC=%errorlevel%"
pause
exit /b %RC%
