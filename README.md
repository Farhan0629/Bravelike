# Bravelike

> A Windows-first, privacy-focused browser project built for a final-year engineering project.

Bravelike is an original C++20 browser shell and privacy engine built on the **Chromium Embedded Framework (CEF)**. It uses Chromium through CEF rather than attempting to compile or fork Chromium locally. The project is not affiliated with Brave Software, Google, Chromium, or Chrome.

## Status at a glance

**Current milestone:** a functional, single-window Windows browser foundation with request blocking and in-memory per-site Shields.

| Area | Current state |
| --- | --- |
| Privacy core | Implemented and covered by dependency-free tests |
| Windows browser shell | Implemented with CEF Views |
| Request filtering | Connected to CEF request interception |
| Per-site Shields | Implemented in memory for exact HTTP(S) hosts |
| Windows CI packaging | Implemented; builds a downloadable runtime artifact |
| Interactive GUI validation | Still required on a real Windows machine and real sites |
| Tabs, downloads, history, bookmarks, settings, installer | Not implemented yet |

The GitHub Actions workflow compiles, links, packages, and checks required runtime files. That is **not** a replacement for manually testing the GUI on Windows.

---

## What Bravelike does today

### Browser window and navigation

The browser currently provides one CEF Views window with:

- **Back** and **Forward** controls, enabled only when history allows them.
- **Reload** and **Stop** controls that reflect the page loading state.
- A **Home** control that returns to the configured startup page.
- An accessible address/search field. Press **Enter** to navigate.
- Main-frame URL synchronization: the address field updates as navigation completes.
- Browser-title synchronization: page titles appear as `Page title - Bravelike`.
- A profile/cache directory stored next to the executable.

### Address and search behavior

Address input is resolved by the standalone navigation module:

| Input | Result |
| --- | --- |
| `example.com` | Navigates to `https://example.com` |
| `https://example.com/path` | Keeps the explicit scheme and navigates there |
| `best privacy browser` | Creates a URL-encoded Brave Search query |

### Keyboard shortcuts

The browser-content keyboard handler supports:

| Shortcut | Action |
| --- | --- |
| `Ctrl+L` | Focus and select the complete address bar |
| `Ctrl+R` | Reload the active page |
| `Alt+Left` | Go back when history is available |
| `Alt+Right` | Go forward when history is available |
| `Alt+Home` | Open the startup page |
| `Escape` | Stop the current load |
| `Enter` in the address field | Navigate using the address/search resolver |

These shortcuts compile and package successfully in CI. They still need interactive validation on Windows with real sites.

### Privacy filtering and Shields

Every intercepted CEF resource request is evaluated by `FilterEngine` when Shields are enabled for the active site. A matching blocking decision cancels the request through CEF.

The UI includes a **Shields** button:

- Shields are **on by default** for every HTTP(S) host.
- Clicking Shields toggles policy for the current exact normalized host and reloads the page.
- A choice for `www.example.com` is distinct from `example.com` because policy is keyed by the exact host.
- Returning to a host restores its in-memory choice during that browser session.
- Non-HTTP(S) pages have no active host; the button reads **`Shields: N/A`** and is disabled.
- The label shows a **cumulative** blocked-request count while Shields are on.

### Supported rule syntax

The current engine deliberately implements a small, auditable subset—not full EasyList compatibility.

```text
# Blank lines and comments are ignored
ads.example.com
||tracker.example^
@@allowed.tracker.example
```

| Rule | Meaning |
| --- | --- |
| `example.com` | Blocks the domain and all of its subdomains |
| `||example.com^` | Accepted as a domain-block rule for migration convenience |
| `@@example.com` | Creates an allow rule |
| Allow + block match | Allow rule wins |

Each evaluation returns an allow/block action, a reason, and the matched source rule when applicable. Unsupported syntax is allowed rather than silently over-blocked.

---

## Architecture

```text
+----------------------+       main-frame URL / load / title events
| CEF Views window     | <-------------------------------------------+
| toolbar + browser    |                                             |
+----------+-----------+                                             |
           | address input, buttons, shortcuts                        |
           v                                                          |
+----------------------+                                               |
| BrowserWindow        |                                               |
| - navigation state   |                                               |
| - Shields UI         |                                               |
| - CEF handlers       |                                               |
+----------+-----------+                                               |
           | request URL                                               |
           v                                                           |
+----------------------+     decision      +------------------------+ |
| FilterEngine         | ----------------> | cancel or continue CEF | |
| independent core     |                   | resource request       | |
+----------+-----------+                   +------------------------+ |
           |                                                              
           +--> PrivacyStats (atomic counters)
           +--> SiteShields (thread-safe active-host policy)

Address/search input --> ResolveAddressInput --> URL or Brave Search URL
```

### Why the core is separate from CEF

`src/core` has no CEF dependency. This keeps filter, URL, navigation, policy, and statistics behavior fast to build and test on their own. `src/browser` is the Windows/CEF adapter that turns core decisions into browser behavior.

### Privacy decision flow

1. CEF starts a resource request.
2. `BrowserWindow::OnBeforeResourceLoad` checks whether Shields are enabled for the active page host.
3. If enabled, `FilterEngine::Evaluate()` processes the request URL.
4. A matching block rule returns a blocking decision; CEF receives `RV_CANCEL`.
5. `PrivacyStats` records every evaluated request and whether it was blocked.

---

## Repository map

This table is the quickest guide to what each part of the project owns.

| Location | Responsibility |
| --- | --- |
| `CMakeLists.txt` | C++20 project setup, core library, tests, optional Windows CEF browser build, pinned CEF configuration |
| `config/sample-blocklist.txt` | Example rules copied into the runtime output as `config/blocklist.txt` |
| `cmake/DownloadCEF.cmake` | Downloads, checksum-verifies, and extracts the pinned CEF binary distribution |
| `src/core/filter_engine.*` | Rule loading, domain/subdomain matching, allow precedence, explainable decisions |
| `src/core/navigation.*` | URL detection, HTTPS normalization, Brave Search fallback, query encoding |
| `src/core/url_utils.*` | ASCII host normalization, HTTP(S) host extraction, domain/subdomain checks |
| `src/core/site_shields.*` | Thread-safe active-host Shields policy and in-memory disabled-host set |
| `src/core/privacy_stats.*` | Thread-safe evaluated and blocked request counters |
| `src/app/filter_demo.cpp` | Command-line tool for loading a rule file and interactively evaluating URLs |
| `src/browser/main_win.cpp` | Windows entry point and CEF process startup |
| `src/browser/browser_app.*` | CEF application/lifecycle configuration |
| `src/browser/browser_window.*` | Window, toolbar, keyboard shortcuts, CEF callbacks, request cancellation, Shields UI |
| `src/browser/CMakeLists.txt` | Browser target, CEF wrapper linking, runtime/resource copying, rule-file copying |
| `tests/filter_engine_tests.cpp` | Filter, allow-rule, URL, navigation, and stats tests |
| `tests/site_shields_tests.cpp` | Host normalization and per-site Shields policy tests |
| `docs/ARCHITECTURE.md` | Design goals and security boundaries |
| `docs/BUILDING_WINDOWS.md` | Windows prerequisites, build commands, and manual smoke-test checklist |
| `docs/ROADMAP.md` | Completed work, future milestones, and deliberate non-goals |
| `scripts/check-windows-prerequisites.ps1` | Basic Git/CMake and compiler-environment check |
| `.github/workflows/core-ci.yml` | Ubuntu/Windows core CI plus Windows CEF packaging and artifact upload |

---

## Requirements

### Required for the Windows browser

- Windows 11, 64-bit
- Visual Studio 2022 with **Desktop development with C++**
  - MSVC v143 x64/x86 build tools
  - Windows 10/11 SDK
  - C++ CMake tools for Windows
- CMake **3.24+**
- Git for Windows
- Internet access for the first CEF configuration
- At least **20 GB free disk space** for CEF, build output, symbols, and experiments

The core library and its tests can also be built on Linux/macOS. The CEF desktop browser target is Windows-only.

### Pinned browser dependency

The CEF configuration automatically downloads the official 64-bit Windows minimal distribution and verifies its checksum:

- **CEF:** `152.0.7+g83ffcba+chromium-152.0.7977.83`
- **Chromium:** `152.0.7977.83`

CEF is prebuilt. Do not try to build Chromium locally as part of this project.

---

## Getting started

### 1. Clone the repository

Open **Developer PowerShell for Visual Studio 2022** and run:

```powershell
git clone https://github.com/Farhan0629/Bravelike.git
cd Bravelike
```

Optional prerequisite check:

```powershell
Set-ExecutionPolicy -Scope Process Bypass -Force
.\scripts\check-windows-prerequisites.ps1
```

### 2. Build and test the independent privacy core

This does not download or require CEF.

```powershell
cmake -S . -B out\build -G "Visual Studio 17 2022" -A x64 -DBRAVELIKE_BUILD_TESTS=ON
cmake --build out\build --config Release
ctest --test-dir out\build -C Release --output-on-failure
```

Run the command-line filter demo:

```powershell
.\out\build\Release\bravelike_filter_demo.exe config\sample-blocklist.txt
```

Then enter URLs one per line. The program prints `ALLOW` or `BLOCK`, an explanation, and the matching rule when one exists. Press `Ctrl+Z`, then `Enter`, to finish in Windows PowerShell.

### 3. Build the Windows browser

The first configure step downloads and extracts CEF, so it can take several minutes.

```powershell
cmake -S . -B out\cef `
  -G "Visual Studio 17 2022" -A x64 `
  -DBRAVELIKE_ENABLE_CEF=ON `
  -DUSE_SANDBOX=OFF

cmake --build out\cef --config Release --target bravelike_browser
```

### 4. Run the browser

```powershell
.\out\cef\Release\bravelike_browser.exe
```

Keep the complete `out\cef\Release` directory together. CEF needs the executable, DLLs, `.pak` resources, locale/runtime data, and `config\blocklist.txt` beside one another.

At minimum, check that the output contains:

```text
bravelike_browser.exe
libcef.dll
chrome_elf.dll
icudtl.dat
resources.pak
config\blocklist.txt
```

---

## Manual Windows smoke test

Perform this before calling a build release-ready. Prefer real HTTPS sites such as YouTube and other normal browsing targets rather than relying only on `example.com`.

1. Launch `bravelike_browser.exe`; confirm it opens without a crash dialog.
2. Confirm the startup page loads.
3. Type a hostname, such as `youtube.com`, then press Enter; confirm HTTPS navigation.
4. Enter a multi-word search; confirm it reaches a Brave Search results page.
5. Visit another page, then verify Back and Forward enable and work.
6. Check Reload, Stop during an active load, and Home.
7. Confirm the address field follows main-frame navigation.
8. Test `Ctrl+L`, `Ctrl+R`, `Alt+Left`, `Alt+Right`, `Alt+Home`, and Escape.
9. On an HTTP(S) site, toggle Shields off and on; each change should reload the page and update the label.
10. Turn Shields off for site A; visit site B and confirm Shields is on there; return to A and confirm the off setting remains for this session.
11. Load a page that requests a configured blocked domain; with Shields on, confirm the cumulative counter increases. Repeat with Shields off and confirm that request does not increase the counter.
12. Open a non-HTTP(S) page and confirm Shields reads `N/A` and is disabled.
13. Close the window and confirm the process exits without hanging.

---

## Tests and continuous integration

### Local tests

The core tests cover, among other behavior:

- exact-domain and subdomain blocking;
- allow-rule precedence;
- host normalization and HTTP/HTTPS handling;
- search/query URL resolution and encoding;
- thread-safe privacy statistics;
- per-site Shields enable/disable behavior;
- host switching and non-HTTP(S) policy behavior.

### GitHub Actions

`.github/workflows/core-ci.yml` runs on every push and pull request:

1. Core configure/build/test on `ubuntu-latest`.
2. Core configure/build/test on `windows-2022`.
3. Windows CEF browser configuration, compilation, runtime-file verification, and artifact upload.

The Windows artifact is named `bravelike-windows-release` and is retained for seven days. It is a CI build artifact, not an installer or proof of interactive GUI behavior.

---

## Known limitations and important safety notes

This is an engineering project in active development, not a production browser. The following limits are intentional and should be understood before extending or distributing it:

- **Single window, no tabs:** tab lifecycle and a new-tab page are future work.
- **No persistence:** per-site Shields choices reset when the browser restarts.
- **Active-page policy snapshot:** during cross-site navigation, a request can briefly use the previous page’s policy. Future work must scope request policy per browser/tab.
- **Cumulative counter:** the displayed blocked number is not per-site.
- **No full EasyList support:** only the documented domain-style subset is supported.
- **Sandbox disabled:** the current local/CI build passes `-DUSE_SANDBOX=OFF`. Enable and validate the CEF sandbox before any public release.
- **No downloads, history, bookmarks, session restore, settings, or installer** yet.
- **No claims of complete tracker blocking, anonymity, or production-grade security** should be made.

Do not commit generated content such as `out/`, `third_party/cef/`, CEF archives, or browser profile/cache data.

---

## Contributing

Contributions should keep the privacy core independently testable and avoid mixing unrelated changes.

1. Start from the latest `main` branch.
2. Create a focused branch, for example:

   ```powershell
   git checkout main
   git pull --ff-only
   git checkout -b feature/short-description
   ```

3. Make a small, focused change.
4. Add or update a core test when behavior in `src/core` changes.
5. Run the applicable local build and tests.
6. For `src/browser` changes, build the CEF target on Windows and document any manual testing performed.
7. Open a pull request describing:
   - what changed;
   - why it changed;
   - automated validation performed;
   - manual Windows validation performed or still required;
   - limitations or follow-up work.

### Engineering guidelines

- Keep `src/core` free from CEF types and APIs.
- Prefer clear, explainable filter decisions over clever but opaque matching.
- Default to allowing syntax the engine does not support; do not silently over-block.
- Treat privacy controls as security-sensitive: document policy scope, persistence, and failure modes.
- Do not claim a feature works solely because it compiles; distinguish CI validation from real Windows GUI validation.
- Keep dependency versions pinned and verify downloads.

---

## Roadmap

Near-term work is tracked in [docs/ROADMAP.md](docs/ROADMAP.md). The main planned areas are:

1. real tabs and a new-tab page;
2. tab-scoped request policy and persisted per-site Shields choices;
3. download confirmation/status;
4. SQLite history and bookmarks;
5. session restoration;
6. rule explanations/categories and safe filter-list updates;
7. settings, installer, benchmarks, threat model, and project evaluation material.

Explicit non-goals for the first release include VPN infrastructure, cryptocurrency custody, account synchronization, a custom rendering engine, and claims of complete EasyList compatibility.

---

## Name and trademark notice

**Bravelike** is a development codename. Use original branding before any public distribution. Brave, Chromium, Chrome, and related marks belong to their respective owners.