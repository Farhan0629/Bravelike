# Building on Windows 11

## Prerequisites

Install:

1. Visual Studio 2022 Community with **Desktop development with C++**.
2. CMake 3.24 or later.
3. Git for Windows.
4. The Windows 10/11 SDK included by Visual Studio.

Run `scripts/check-windows-prerequisites.ps1` from PowerShell to verify the command-line tools.

## Build the privacy core first

```powershell
cmake -S . -B out/build -G "Visual Studio 17 2022" -A x64
cmake --build out/build --config Release
ctest --test-dir out/build -C Release --output-on-failure
```

This build does not require CEF.

## Enable the browser shell

Download a compatible **Windows 64-bit standard CEF binary distribution** from the official CEF project source. Extract it outside the repository, for example `C:\dev\cef`.

```powershell
cmake -S . -B out/cef-build `
  -G "Visual Studio 17 2022" -A x64 `
  -DBRAVELIKE_ENABLE_CEF=ON `
  -DCEF_ROOT=C:\dev\cef
cmake --build out/cef-build --config Release
```

CEF changes frequently. The binary version will be pinned only after a Windows CI build verifies the exact package. Do not commit the extracted CEF distribution.

## Hardware guidance

The core build is lightweight. A prebuilt CEF package is much more practical on a 16 GB laptop than compiling Chromium. Keep at least 20 GB free for CEF packages, build outputs, symbols, and packaging experiments.
