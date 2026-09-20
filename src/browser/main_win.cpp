#if !defined(_WIN32)
#error This shell milestone currently targets Windows.
#endif
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/cef_sandbox_win.h"
#include <windows.h>

namespace {
class BrowserClient final : public CefClient, public CefLifeSpanHandler {
 public:
  CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
  void OnBeforeClose(CefRefPtr<CefBrowser>) override { CefQuitMessageLoop(); }
 private:
  IMPLEMENT_REFCOUNTING(BrowserClient);
};

class BrowserApp final : public CefApp, public CefBrowserProcessHandler {
 public:
  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }
  void OnContextInitialized() override {
    CefWindowInfo window_info;
    window_info.SetAsPopup(nullptr, L"Bravelike");
    CefBrowserSettings settings;
    CefBrowserHost::CreateBrowser(window_info, new BrowserClient(), "https://example.com", settings, nullptr, nullptr);
  }
 private:
  IMPLEMENT_REFCOUNTING(BrowserApp);
};
}

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t*, int) {
  CefMainArgs args(instance);
  void* sandbox_info = nullptr;
  const int subprocess = CefExecuteProcess(args, nullptr, sandbox_info);
  if (subprocess >= 0) return subprocess;
  CefSettings settings;
  settings.no_sandbox = true;  // Development scaffold only; remove before release.
  CefInitialize(args, settings, new BrowserApp(), sandbox_info);
  CefRunMessageLoop();
  CefShutdown();
  return 0;
}
