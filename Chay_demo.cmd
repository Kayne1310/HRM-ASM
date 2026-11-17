@echo off
setlocal
cd /d "%~dp0"
if not exist "demo-run\employees.txt" (
    if not exist "demo-run" mkdir "demo-run"
    copy /y "examples\demo\*.txt" "demo-run\" >nul
)
"build\chamcong.exe" --data "demo-run"
pause
