# Bravelike

Bravelike is a Windows-first, privacy-focused Chromium browser project for a final-year engineering project. It is being built as an original browser shell and privacy engine on top of the Chromium Embedded Framework (CEF); it is not affiliated with Brave Software or Google.

## Current milestone: privacy core

The repository currently contains a buildable C++20 privacy core with:

- normalized HTTP/HTTPS host extraction;
- domain and subdomain blocking;
- allow rules using an `@@` prefix;
- a small, documented filter-list format;
- thread-safe privacy statistics;
- a command-line demonstration program;
- dependency-free unit tests;
- a gated Windows CEF shell scaffold.

The filter core builds without downloading Chromium or CEF, so development can start on a 16 GB Windows 11 laptop.

## Quick start

### Windows 11

Open **Developer PowerShell for Visual Studio 2022**:

```powershell
cmake -S . -B out/build -G "Visual Studio 17 2022" -A x64
cmake --build out/build --config Release
ctest --test-dir out/build -C Release --output-on-failure
.\out\build\Release\bravelike_filter_demo.exe config\sample-blocklist.txt
```

### Linux/macOS core-only verification

```bash
cmake -S . -B out/build
cmake --build out/build
ctest --test-dir out/build --output-on-failure
./out/build/bravelike_filter_demo config/sample-blocklist.txt
```

Then enter URLs one per line. The program prints `BLOCK` or `ALLOW` and the matching rule.

## Rule format

The first milestone intentionally supports a small auditable subset:

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
config/                 Sample local privacy rules
docs/                   Architecture, Windows setup, and roadmap
scripts/                Windows environment checks
src/core/               Standalone privacy engine
src/app/                CLI demonstration
src/browser/            CEF browser-shell scaffold
tests/                  Dependency-free tests
```

## Build philosophy

1. Keep the privacy engine independent from the UI and Chromium integration.
2. Test filtering behavior before connecting it to live requests.
3. Compile against a prebuilt CEF distribution instead of compiling Chromium locally.
4. Make privacy decisions explainable by returning the matched rule.
5. Add browser features in small, reviewable milestones.

See [docs/ROADMAP.md](docs/ROADMAP.md) for planned work and [docs/BUILDING_WINDOWS.md](docs/BUILDING_WINDOWS.md) for Windows prerequisites.

## Name and trademark note

“Bravelike” is a development codename. The project must use original branding before public distribution. Brave, Chromium, Chrome, and related marks belong to their respective owners.
