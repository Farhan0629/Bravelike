#pragma once
#include "include/cef_app.h"

namespace bravelike {
class BrowserApp final : public CefApp, public CefBrowserProcessHandler {
 public:
  BrowserApp() = default;
  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }
  void OnContextInitialized() override;
 private:
  IMPLEMENT_REFCOUNTING(BrowserApp);
  DISALLOW_COPY_AND_ASSIGN(BrowserApp);
};
}  // namespace bravelike
