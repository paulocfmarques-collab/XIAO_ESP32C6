@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0firmware.ps1" %*
exit /b %ERRORLEVEL%
