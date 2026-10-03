#include "src/browser/browser_app.h"
#include "src/browser/browser_window.h"
#include "include/wrapper/cef_helpers.h"

namespace bravelike {
BrowserApp::BrowserApp(std::string startup_url)
    : startup_url_(std::move(startup_url)) {}

void BrowserApp::OnContextInitialized() {
  CEF_REQUIRE_UI_THREAD();
  const std::string url = startup_url_.empty() ? BrowserWindow::DefaultHomeUrl() : startup_url_;
  BrowserWindow::Create(url);
}
}  // namespace bravelike
