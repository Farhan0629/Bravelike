#include "src/browser/browser_app.h"
#include "src/browser/browser_window.h"
#include "include/wrapper/cef_helpers.h"

namespace kingfn {
BrowserApp::BrowserApp(std::string startup_url)
    : startup_url_(std::move(startup_url)) {}

void BrowserApp::OnBeforeCommandLineProcessing(
    const CefString& process_type,
    CefRefPtr<CefCommandLine> command_line) {
  // ── Privacy & security hardening ──────────────────────────────────────────
  // Disable features that phone home or enable remote control surfaces.
  command_line->AppendSwitch("disable-background-networking");
  command_line->AppendSwitch("disable-client-side-phishing-detection");
  command_line->AppendSwitch("disable-component-update");
  command_line->AppendSwitch("disable-default-apps");
  command_line->AppendSwitch("disable-extensions");
  command_line->AppendSwitch("disable-sync");
  command_line->AppendSwitch("disable-translate");
  command_line->AppendSwitch("metrics-recording-only");
  command_line->AppendSwitch("no-first-run");
  command_line->AppendSwitch("no-pings");
  command_line->AppendSwitch("safebrowsing-disable-auto-update");

  // ── GPU crash prevention (Intel Iris Xe & integrated GPUs) ───────────────
  // Prefer disabling only GPU compositing (software rasterizer) rather than
  // the whole GPU stack, which would also kill WebGL and hardware video decode.
  // Fall back to full --disable-gpu only if the user hasn't opted into GPU.
  if (!command_line->HasSwitch("enable-gpu")) {
    command_line->AppendSwitch("disable-gpu-compositing");
    // Also force the swiftshader path on systems with known-problematic drivers.
    command_line->AppendSwitch("use-gl");
    command_line->AppendSwitchWithValue("use-gl", "swiftshader-webgl");
  }

  // ── Media & codec flags for YouTube / general video playback ─────────────
  command_line->AppendSwitch("autoplay-policy=no-user-gesture-required");
  command_line->AppendSwitch("enable-features=VaapiVideoDecodeLinuxGL");

  // ── Misc UX improvements ──────────────────────────────────────────────────
  command_line->AppendSwitch("disable-popup-blocking");
  command_line->AppendSwitch("disable-hang-monitor");
}

void BrowserApp::OnContextInitialized() {
  CEF_REQUIRE_UI_THREAD();
  const std::string url = startup_url_.empty() ? BrowserWindow::DefaultHomeUrl() : startup_url_;
  BrowserWindow::Create(url);
}
}  // namespace kingfn
