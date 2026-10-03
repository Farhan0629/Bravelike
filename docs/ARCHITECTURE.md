# Architecture and security boundaries

## Implemented data flow

`BrowserWindow` owns the single browser View, toolbar, `FilterEngine`, `SiteShields`, and `PrivacyStats`. UI-thread address/title/loading callbacks update the controls. CEF resource callbacks evaluate request URLs and return RV_CANCEL for matching blocked requests when active-host Shields are enabled.

The core has no CEF dependency. Rule lists are loaded before live requests; concurrent rule mutation/hot reload is not implemented. Site policy uses a mutex, statistics use atomics. The two counters are separately atomic, not a transactionally consistent pair.

The active-site snapshot is synchronized but not scoped to each navigation or renderer context. Redirect/cross-site requests can use previous-site policy briefly. Tabs and popup lifecycle require a redesign rather than sharing this snapshot globally.

## Home-page identity

The home page remains a file URL. `home_policy.h` escapes Windows UTF-8 paths; `home_url.h` canonicalizes through CEF. Only exact equality with the owned canonical home URL permits a blank address field. Internal aliases are UI routing shortcuts, not privileged registered schemes. The page contains no native bridge or verified live security/status metrics.

Home navigation is separate from startup navigation, preventing a command-line website from redefining the Home destination.

## Sandbox

CEF 152 follows the M138+ Windows bootstrap model: official bootstrap EXE plus client DLL exporting RunWinMain. CEF supplies the sandbox pointer, passed to CefExecuteProcess and CefInitialize. The sandbox configuration rejects absent context and --no-sandbox rather than quietly weakening isolation. Runtime token/process isolation still needs verification on Windows; a package build alone does not prove it.

An explicit USE_SANDBOX=OFF configuration remains development-only. Consult official setup: https://chromiumembedded.github.io/cef/sandbox_setup

## Remaining release risks

External protocol confirmation, popup/download lifecycle, permission prompts, local/internal navigation boundaries, certificate-error behavior, profile storage policy, rule update integrity, sandbox runtime evidence, crash handling, signed updates and traffic auditing need design/testing. Never treat domain blocking as anonymity or complete tracking prevention. Changing GPU switches is a compatibility workaround, not a security control.
