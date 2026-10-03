#pragma once
#include <string>
#include "include/cef_app.h"

namespace kingfn {
class BrowserApp final : public CefApp, public CefBrowserProcessHandler {
 public:
  explicit BrowserApp(std::string startup_url = "");
  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }
  void OnBeforeCommandLineProcessing(const CefString& process_type,
                                     CefRefPtr<CefCommandLine> command_line) override;
  void OnContextInitialized() override;
 private:
  std::string startup_url_;
  IMPLEMENT_REFCOUNTING(BrowserApp);
  DISALLOW_COPY_AND_ASSIGN(BrowserApp);
};
}  // namespace kingfn
