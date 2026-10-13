@echo off
title KINGFN Browser
cd /d "%~dp0"

set "EXE=out\ci-release\kingfn_browser.exe"

if not exist "%EXE%" (
    echo.
    echo  [ERROR] kingfn_browser.exe not found at: %EXE%
    echo  Please make sure the build output is present.
    pause
    exit /b 1
)

echo  Launching KINGFN Browser v0.3.0...
start "" "%EXE%"
