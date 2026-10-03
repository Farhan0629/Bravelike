@echo off
setlocal
cd /d "%~dp0"

set "EXE=out\ci-release\bravelike_browser.exe"
if not exist "%EXE%" (
    set "EXE=out\cef\Release\bravelike_browser.exe"
)

if not exist "%EXE%" (
    echo [ERROR] Bravelike browser executable not found.
    echo Please make sure the build is completed in out\ci-release.
    pause
    exit /b 1
)

echo Starting Bravelike Browser...
start "" "%EXE%" %*
