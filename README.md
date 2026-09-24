# Bravelike

Bravelike is a Windows-first, privacy-focused Chromium browser project for a final-year engineering project. It is being built as an original browser shell and privacy engine on top of the Chromium Embedded Framework (CEF); it is not affiliated with Brave Software or Google.

## Current milestone: functional Windows browser foundation

The repository now contains:

- a Windows CEF Views browser window;
- working Back, Forward, Reload, Stop, and Home controls;
- an address/search field with Enter navigation;
- URL normalization and Brave Search fallback for plain text;
- a native request-interception path connected to the privacy filter engine;
- a Shields toggle that reloads the page and displays the cumulative blocked-request count;
- window-title, address, loading-state, and navigation-state synchronization;
- a persistent profile/cache directory beside the executable;
- normalized HTTP/HTTPS host extraction, domain/subdomain blocking, `@@` allow rules, and explainable decisions;
- thread-safe privacy statistics and dependency-free unit tests;
- automated Windows CEF compilation, runtime-file verification, and a downloadable CI artifact.

This milestone is intentionally a single-window browser foundation. Tabs, history, bookmarks, downloads UI, settings, session restore, and an installer remain later milestones.

## Quick start

### Windows 11 core tests

Open **Developer PowerShell for Visual Studio 2022**:

```powershell
cmake -S . -B out/build -G "Visual Studio 17 2022" -A x64 -DBRAVELIKE_BUILD_TESTS=ON
cmake --build out/build --config Release
ctest --test-dir out/build -C Release --output-on-failure
.\out\build\Release\bravelike_filter_demo.exe config\sample-blocklist.txt
```

### Windows 11 browser

The browser build automatically downloads and checksum-verifies the pinned official Windows 64-bit minimal CEF distribution:

- CEF `152.0.7+g83ffcba+chromium-152.0.7977.83`
- Chromium `152.0.7977.83`

```powershell
cmake -S . -B out/cef `
  -G "Visual Studio 17 2022" -A x64 `
  -DBRAVELIKE_ENABLE_CEF=ON `
  -DUSE_SANDBOX=OFF
cmake --build out/cef --config Release --target bravelike_browser
.\out\cef\Release\bravelike_browser.exe
```

The first configuration downloads CEF and can take several minutes. Extracted dependencies and build outputs must not be committed.

### Linux/macOS core-only verification

```bash
cmake -S . -B out/build
cmake --build out/build
ctest --test-dir out/build --output-on-failure
./out/build/bravelike_filter_demo config/sample-blocklist.txt
```

## Rule format

The current milestone intentionally supports a small auditable subset:

```text
# comments and blank lines are ignored
ads.example.com
||tracker.example^
@@allowed.tracker.example
```

- `example.com` blocks that domain and its subdomains.
- `||example.com^` is accepted as a domain rule for migration convenience.
- `@@example.com` creates an allow rule.
- Allow rules take precedence over block rules.

Full EasyList compatibility is **not** claimed.

## Project structure

```text
config/                 Local privacy rules copied beside the browser
docs/                   Architecture, Windows setup, and roadmap
scripts/                Windows environment checks
src/core/               Standalone privacy and navigation core
src/app/                CLI demonstration
src/browser/            CEF Views browser application
tests/                  Dependency-free tests
```

## Build philosophy

1. Keep the privacy engine independent from the UI and Chromium integration.
2. Test filtering and navigation behavior before connecting them to live requests.
3. Compile against a pinned prebuilt CEF distribution instead of compiling Chromium locally.
4. Make privacy decisions explainable by returning the matched rule.
5. Add browser features in small, reviewable milestones.

See [docs/ROADMAP.md](docs/ROADMAP.md) for planned work and [docs/BUILDING_WINDOWS.md](docs/BUILDING_WINDOWS.md) for Windows prerequisites and runtime validation.

## Name and trademark note

“Bravelike” is a development codename. The project must use original branding before public distribution. Brave, Chromium, Chrome, and related marks belong to their respective owners.
