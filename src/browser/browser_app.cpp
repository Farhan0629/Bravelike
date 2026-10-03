#include "src/browser/browser_app.h"
#include "src/browser/browser_window.h"
#include "include/wrapper/cef_helpers.h"
namespace kingfn {
BrowserApp::BrowserApp(std::string startup_url) : startup_url_(std::move(startup_url)) {}
void BrowserApp::OnBeforeCommandLineProcessing(const CefString& process_type,
    CefRefPtr<CefCommandLine> command_line) {
  // Opt-in compatibility mode; this is not hardware detection or crash recovery.
  // Chromium also accepts --disable-gpu directly.
  if (command_line->HasSwitch("gpu-compatibility")) {
    command_line->AppendSwitch("disable-gpu");
  }
}
void BrowserApp::OnContextInitialized() {
  CEF_REQUIRE_UI_THREAD();
  BrowserWindow::Create(startup_url_.empty() ? BrowserWindow::DefaultHomeUrl() : startup_url_);
}
}
