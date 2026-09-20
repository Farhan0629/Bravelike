#include "src/browser/browser_app.h"
#include "src/browser/browser_window.h"
#include "include/wrapper/cef_helpers.h"

namespace bravelike {
void BrowserApp::OnContextInitialized() {
  CEF_REQUIRE_UI_THREAD();
  BrowserWindow::Create("https://search.brave.com");
}
}  // namespace bravelike
