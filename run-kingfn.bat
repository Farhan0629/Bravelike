@echo off
setlocal
cd /d "%~dp0"

set "EXE=out\ci-release\kingfn_browser.exe"
if not exist "%EXE%" (
    set "EXE=out\cef\Release\kingfn_browser.exe"
)
if not exist "%EXE%" (
    set "EXE=out\ci-release\bravelike_browser.exe"
)

if not exist "%EXE%" (
    echo [ERROR] KINGFN browser executable not found.
    echo Please make sure the build is completed in out\ci-release.
    pause
    exit /b 1
)

echo Starting KINGFN Browser...
start "" "%EXE%" %*
