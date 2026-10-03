# Repeatable Windows validation

Record commit, Windows version, VS/CMake versions, sandbox mode, GPU flags, GPU/driver version, test date and exact URLs. For each item record PASS/FAIL/NOT RUN; never infer runtime correctness from CI. Use a disposable browser profile with no sensitive login data. Do not test malware or intentionally hostile pages.

## Navigation/security regression cases

1. Start with no URL; confirm the local portal loads.
2. Start with `kingfn_browser.exe https://www.youtube.com`; confirm YouTube loads, then Home and Alt+Home return to the local portal, not YouTube.
3. Enter home aliases; verify they return home.
4. Visit a benign controlled URL whose path/query contains `resources/home.html`; its complete external address must stay visible. Do not rely on the site's page title for identity.
5. Copy the full runtime to a writable folder containing spaces and `#`, then verify the home page loads. Also test a non-ASCII username/path.
6. Test address typing, Enter navigation and search; compare the home-page search selector with the address bar's DuckDuckGo-only search behavior.
7. Use Ctrl+L/Ctrl+R/Alt+Left/Alt+Right/Alt+Home from both web content and address-field focus. Check ordinary typing and modified combinations are not hijacked.
8. Check Escape stops an active load and still reaches an idle page's dialog/fullscreen behavior. A CEF fullscreen display implementation may be separately needed; record unsupported behavior rather than marking it passed.
9. Test Back/Forward enabled states, Reload, Stop, title/address synchronization and clean shutdown.

## Redirects, popups and filtering

Use benign sites or a controlled local fixture; do not assume popup support is implemented.

- Navigate through a redirect from host A to B; confirm final URL stays visible and record Shields behavior during the transition. Previous-page policy timing is a known limitation.
- Open a target=_blank link and a user-triggered window.open. Record actual behavior, process/window cleanup and whether the parent toolbar changes incorrectly. Popup lifecycle is not a supported feature until explicitly designed.
- With a controlled page that requests a configured blocked host, confirm cancellation with Shields on and loading with Shields off. Verify an allow rule overrides a matching block rule.
- Toggle A off, visit B (on), return to A (off). Restart and confirm choices reset.
- Confirm non-HTTP(S) pages show N/A. The counter is cumulative and is not refreshed on every individual request.

## Sandbox runtime evidence

Build USE_SANDBOX=ON in a clean directory. Confirm both the renamed official bootstrap EXE and client DLL exist. Launch normally, verify renderer subprocesses start and record their process mitigation/token restrictions using an appropriate Windows inspection tool such as Process Explorer. Record renderer process integrity/restrictions, not just the browser process. Verify launching the sandbox build with --no-sandbox fails. Compare only with an explicitly labeled disposable unsandboxed development build if needed.

If process inspection cannot establish isolation, mark sandbox runtime validation NOT RUN. Successful startup alone is not sufficient.

## GPU/real-site compatibility

Repeat the same YouTube video, resolution, duration, viewport and driver configuration with normal GPU behavior and --gpu-compatibility. Record playback failures, CPU load, dropped frames, memory, and power observations. Avoid claiming quantitative improvements without measurements. Check GitHub navigation and a Wikipedia article as additional browsing cases.

## Release gate

No public release until sandbox evidence, navigation identity, lifecycle, protocol/permission/download safety, third-party notices, authorized project licensing, and known limitations are reviewed. Attach observations to the PR rather than claiming universal site support.
