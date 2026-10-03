<div align="center">
<img src="assets/kingfn-banner.jpg" alt="KINGFN Browser" width="100%" />

# KINGFN Browser

A C++20 Windows browser project built on Chromium Embedded Framework.

[![Core CI](https://github.com/Farhan0629/Bravelike/actions/workflows/core-ci.yml/badge.svg)](https://github.com/Farhan0629/Bravelike/actions/workflows/core-ci.yml)

</div>

## Status and scope

KINGFN is a development prototype, not a production-hardened browser. The maintainer reports successful Windows laptop use. That does not establish compatibility with all sites, security against hostile content, anonymity, or performance advantages over other browsers.

Implemented: a single CEF Views window, Back/Forward/Reload/Stop/Home, address/search input, keyboard navigation, local HTML home page, domain-based request cancellation, in-memory exact-host Shields choices, cumulative statistics, CLI demo, core tests, and Windows CI packaging.

Not implemented: tabs, persistent Shields settings, download manager, history/bookmarks database, session restore, updater, installer, and a live home-page/browser metrics bridge.

This hardening branch adds a CEF bootstrap sandbox build path. It must pass Windows compilation and runtime sandbox validation before being considered release-ready. An explicit unsandboxed development configuration remains available and must not be presented as a safe public release.

## Architecture and behavior

CEF supplies Chromium rendering and network facilities; KINGFN is the browser shell and rule engine, not a new rendering engine or a browser built entirely from scratch. The independently testable `kingfn_core` has no CEF dependency.

```text
Address input -> navigation resolver -> HTTPS URL or DuckDuckGo query
Buttons / shortcuts -> BrowserWindow -> CEF browser actions
CEF resource callback -> active-host Shields policy -> FilterEngine
                     -> cancel / continue -> PrivacyStats
CEF address / title / loading callbacks -> toolbar and window updates
```

### Navigation

- `youtube.com` resolves to `https://youtube.com`.
- Explicit `scheme://` inputs are preserved; the resolver is not a security validator for arbitrary protocols.
- Plain search terms become encoded DuckDuckGo queries.
- `home`, `kingfn`, `kingfn://home`, `kingfn://newtab`, and `about:home` are address-field aliases, not registered internal schemes.
- Home and Alt+Home target the owned local home page independently of command-line startup navigation.
- Only the exact canonical owned home URL is hidden in the address bar. External URLs containing `resources/home.html` remain visible.
- Home file paths are escaped and canonicalized through CEF, including spaces, percent signs, and fragment characters.

### Shortcuts

| Shortcut | Behavior |
| --- | --- |
| Ctrl+L | Focus/select address field |
| Ctrl+R | Reload |
| Alt+Left / Alt+Right | Navigate available history |
| Alt+Home | Owned local home page |
| Escape | Stop an active load; leave idle-site Escape handling available |
| Enter in address field | Navigate/search |

Shortcuts are handled in browser content and the address field; global behavior in every dialog/focus state is not claimed.

### Shields

Shields default on for HTTP(S) hosts. Clicking the toolbar button toggles the exact normalized active host and reloads. Parent/subdomain choices are separate. Choices reset on restart. Non-HTTP(S) pages show `Shields: N/A`.

The number shown is cumulative for the browser session, not per-site. The label refreshes on loading-state/address updates and toggles, not on every intercepted request. The home page contains informational text, not live browser telemetry.

**Remaining limitation:** filtering uses a synchronized active-page snapshot. During navigation/redirects, requests can briefly use the previous host's policy. Browser/tab-scoped policy is required before tabs are introduced.

### Rule syntax

```text
# Whole-line comments; blank lines are ignored
ads.example.com
||tracker.example^
@@allowed.tracker.example
```

Domain rules match the domain and its subdomains, with label boundaries (not arbitrary suffixes). Allow rules take precedence. Unsupported rule lines are skipped and returned as warnings by the loader; the CLI displays those warnings. The browser does not currently expose load warnings. **Inline comments are not supported.** Full EasyList, cosmetic filtering, scriptlets, and guaranteed YouTube ad blocking are not supported.

The browser loads `config/blocklist.txt` next to the executable at startup; rebuilds copy `config/sample-blocklist.txt` over the runtime file. File-open errors trigger a small built-in fallback list. Hot updates are not implemented.

## Repository map

| File / directory | Responsibility |
| --- | --- |
| `CMakeLists.txt` | C++20 targets, tests, option compatibility, CEF dependency configuration |
| `cmake/DownloadCEF.cmake` | Pinned CEF archive download, checksum verification, extraction |
| `src/core/filter_engine.*` | Rule parsing, allow-first domain matching, explainable decisions |
| `src/core/navigation.*` | Address resolution and query encoding |
| `src/core/home_policy.h` | Exact home identification, aliases, escaped Windows file URLs |
| `src/core/url_utils.*` | HTTP(S) host parsing and domain-boundary matching |
| `src/core/site_shields.*` | Mutex-protected in-memory active-host choices |
| `src/core/privacy_stats.*` | Atomic evaluated/blocked counters |
| `src/app/filter_demo.cpp` | Interactive rule evaluation CLI |
| `src/browser/main_win.cpp` | EXE/DLL entry points, CEF subprocess dispatch, profile and sandbox configuration |
| `src/browser/browser_app.*` | Browser lifecycle and opt-in GPU compatibility flag |
| `src/browser/browser_window.*` | Views UI, home navigation, shortcuts, CEF handlers, filtering adapter |
| `src/browser/home_url.h` | CEF canonicalization of the escaped home file URL |
| `src/browser/CMakeLists.txt` | Client DLL/EXE build, bootstrap packaging, runtime/resource copying |
| `src/browser/kingfn.rc` | Windows client branding resources |
| `resources/home.html` | Local search form, quick links, informational development-build notice |
| `resources/icons/`, `assets/` | Branding assets |
| `config/sample-blocklist.txt` | Seed domain rule file |
| `tests/` | Standalone core and navigation-security regression tests |
| `scripts/check-windows-prerequisites.ps1` | Basic tool presence check; not full SDK/toolchain certification |
| `scripts/test-cmake-options.py` | Fresh-cache legacy/new option precedence regression checks |
| `scripts/test-home-page.js` | Dependency-free home-page search and interaction contract checks |
| `.github/workflows/core-ci.yml` | Core tests and sandboxed/unsandboxed Windows package builds |
| `run-kingfn.bat` | Launch an already-built/downloaded runtime; does not build it |
| `run-bravelike.bat` | Legacy launcher |
| `docs/ARCHITECTURE.md` | Design and security boundaries |
| `docs/BUILDING_WINDOWS.md` | Windows build and packaging details |
| `docs/WINDOWS_VALIDATION.md` | Repeatable manual real-site, redirect, popup, GPU and sandbox checks |
| `docs/ROADMAP.md` | Remaining work |

## Windows prerequisites

- Windows x64; the maintainer uses Windows 11, CI uses `windows-2022`. Windows 10 compatibility requires separate validation.
- Visual Studio 2022 Community **or Build Tools 2022**, Desktop development with C++, MSVC v143 x64/x86, Windows 10/11 SDK, C++ CMake tools.
- CMake 3.24+, Git, internet for initial CEF download, and roughly 20 GB free disk as practical development guidance.
- Optional Python 3 and Node.js for the supplementary build/home-page contract checks; not needed for the browser executable or normal C++ core tests.

Open Developer PowerShell for VS 2022. Keep your checkout outside OneDrive when practical to avoid synchronizing large outputs.

```powershell
git clone https://github.com/Farhan0629/Bravelike.git
cd Bravelike
# To try this review branch:
git switch fix/kingfn-hardening-review
```

### Core-only build

```powershell
cmake -S . -B out\core -G "Visual Studio 17 2022" -A x64 -DKINGFN_BUILD_TESTS=ON
cmake --build out\core --config Release
ctest --test-dir out\core -C Release --output-on-failure
.\out\core\Release\kingfn_filter_demo.exe config\sample-blocklist.txt
```

Core-only Linux/macOS:

```bash
cmake -S . -B out/core
cmake --build out/core
ctest --test-dir out/core --output-on-failure
```

### Sandboxed Windows build (pending runtime validation)

Pinned package: CEF `152.0.7+g83ffcba+chromium-152.0.7977.83`, Windows64 minimal; Chromium `152.0.7977.83`.

```powershell
cmake -S . -B out\sandbox -G "Visual Studio 17 2022" -A x64 -DKINGFN_ENABLE_CEF=ON -DUSE_SANDBOX=ON
cmake --build out\sandbox --config Release --target kingfn_browser
.\out\sandbox\Release\kingfn_browser.exe
```

CEF M138+ Windows sandboxing uses an official bootstrap executable loading a client DLL. This build copies `bootstrap.exe` as `kingfn_browser.exe` alongside `kingfn_browser.dll`, whose exported `RunWinMain` receives the sandbox context. Missing bootstrap files stop configuration rather than silently falling back. Passing `--no-sandbox` to the sandbox client is rejected. The bootstrap EXE retains upstream resources unless separately customized; the client DLL's branding resource does not automatically rebrand that EXE.

Do not copy only the executable: keep DLLs, all copied runtime resources/locales, `config/`, and `resources/` together. See [Windows build details](docs/BUILDING_WINDOWS.md).

### GPU compatibility mode

GPU acceleration is no longer disabled globally. If your laptop needs the previous workaround:

```powershell
.\out\sandbox\Release\kingfn_browser.exe --gpu-compatibility
```

`--disable-gpu` is also accepted by Chromium. This is an explicit workaround, not Intel-specific detection or automatic crash recovery. Compare video playback, CPU, dropped frames and battery behavior before choosing a default for your machine.

### Unsandboxed development fallback

Use a separate build directory. This configuration is for debugging, not public distribution:

```powershell
cmake -S . -B out\dev -G "Visual Studio 17 2022" -A x64 -DKINGFN_ENABLE_CEF=ON -DUSE_SANDBOX=OFF
cmake --build out\dev --config Release --target kingfn_browser
```

### Download instead of compiling

A source clone contains no browser binaries. Open the repository's Actions page, select a successful run for the intended commit, download the sandboxed runtime artifact, and extract **the entire artifact** into `out/ci-release/` before invoking `run-kingfn.bat`. Artifacts expire after seven days. A CI artifact is not an installer, signed release, or proof of interactive correctness.

## Validation and contributing

Run CTest for core changes. CI additionally exercises fresh CMake option caches and home-page search behavior, and compiles both Windows packaging modes. The Windows jobs verify files, not actual renderer isolation or GUI interactions. Use [the Windows validation checklist](docs/WINDOWS_VALIDATION.md) and record exact commit, build mode, GPU flag, OS, site URLs and observations.

Create a focused branch, add regression tests, keep CEF types outside the core, and open a PR stating automated/manual checks and unverified behavior. Never replace missing runtime evidence with a green build badge. Do not commit `out/`, downloaded CEF, archives or browsing data. `KINGFNProfile` beside the executable stores persistent cookies/cache; it is not private-browsing mode, encrypted isolation, or zero network activity. Back up before deleting a profile.

## Privacy, licensing and release boundaries

No application analytics service is configured in the reviewed implementation. **Zero telemetry is not established:** CEF/Chromium and websites can initiate network activity; a traffic audit and a precise privacy statement are still required. There is no measured speed/memory advantage claim.

The former MIT badge was removed because no project license file was present. **A license has not been chosen in this branch.** The maintainer must choose/authorize one; do not assume permission for unrestricted redistribution. CEF and bundled third-party software have separate license/notice obligations which must be preserved in any distribution.

KINGFN is an independent project, not affiliated with Brave or Google. Chromium/Chrome/Brave and website names remain their owners' marks. See [roadmap](docs/ROADMAP.md) for tabs, persistence, downloads, history, benchmarks and release hardening.
