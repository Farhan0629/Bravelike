# Building on Windows 11

## Prerequisites

Install:

1. Visual Studio 2022 Community with **Desktop development with C++**.
2. CMake 3.24 or later.
3. Git for Windows.
4. The Windows 10/11 SDK included by Visual Studio.

Run `scripts/check-windows-prerequisites.ps1` from PowerShell to verify the command-line tools.

## Build and test the privacy core

```powershell
cmake -S . -B out/build -G "Visual Studio 17 2022" -A x64 -DBRAVELIKE_BUILD_TESTS=ON
cmake --build out/build --config Release
ctest --test-dir out/build -C Release --output-on-failure
```

This build does not require CEF.

## Build the Windows browser

The project pins the official Windows 64-bit minimal distribution:

- CEF `152.0.7+g83ffcba+chromium-152.0.7977.83`
- Chromium `152.0.7977.83`

Configuration downloads the checksum and archive, verifies the archive, and extracts it under `third_party/cef`.

```powershell
cmake -S . -B out/cef `
  -G "Visual Studio 17 2022" -A x64 `
  -DBRAVELIKE_ENABLE_CEF=ON `
  -DUSE_SANDBOX=OFF
cmake --build out/cef --config Release --target bravelike_browser
```

Run:

```powershell
.\out\cef\Release\bravelike_browser.exe
```

The browser output directory must include at least:

```text
bravelike_browser.exe
libcef.dll
chrome_elf.dll
icudtl.dat
resources.pak
config/blocklist.txt
```

Do not move only the executable; CEF requires the adjacent runtime binaries and resources. Do not commit `third_party/cef`, `out`, the generated profile directory, or downloaded archives.

## Functional smoke test

GitHub Actions validates compilation, linking, runtime-file copying, and artifact creation. It cannot interact with the desktop GUI, so run this checklist on Windows 11 before declaring a release candidate:

1. Start `bravelike_browser.exe` and confirm the window opens without a console or crash dialog.
2. Confirm the Brave Search home page loads.
3. Enter `example.com` and confirm it resolves to `https://example.com`.
4. Enter plain search text and confirm a Brave Search results page loads.
5. Navigate to a second page and verify Back and Forward enable and work.
6. Verify Reload refreshes, Stop interrupts an active load, and Home returns to the start page.
7. Verify the address field tracks main-frame navigation.
8. Toggle Shields off and on; each toggle must reload and update the label.
9. Load a page that requests a configured blocked domain and confirm the blocked count increases.
10. Close the window and confirm the process exits without hanging.

## Hardware guidance

The core build is lightweight. A prebuilt CEF package is much more practical on a 16 GB laptop than compiling Chromium. Keep at least 20 GB free for CEF packages, build outputs, symbols, and packaging experiments. Integrated graphics are sufficient for this milestone.
