@echo off
if not exist "%~dp0install.ps1" (
  echo Extract the whole ZIP first, then run INSTALL.bat.
  pause
  exit /b 1
)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0install.ps1" %*
set "RC=%errorlevel%"
pause
exit /b %RC%
