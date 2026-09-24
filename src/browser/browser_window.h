#pragma once
#include <string>

#include "include/cef_client.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_browser_view_delegate.h"
#include "include/views/cef_button_delegate.h"
#include "include/views/cef_label_button.h"
#include "include/views/cef_textfield.h"
#include "include/views/cef_textfield_delegate.h"
#include "include/views/cef_window.h"
#include "include/views/cef_window_delegate.h"
#include "src/core/filter_engine.h"
#include "src/core/privacy_stats.h"
#include "src/core/site_shields.h"

namespace bravelike {
class BrowserWindow final : public CefClient,
                            public CefDisplayHandler,
                            public CefLifeSpanHandler,
                            public CefLoadHandler,
                            public CefRequestHandler,
                            public CefResourceRequestHandler,
                            public CefBrowserViewDelegate,
                            public CefButtonDelegate,
                            public CefTextfieldDelegate,
                            public CefWindowDelegate {
 public:
  static void Create(const std::string& startup_url);

  CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
  CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
  CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
  CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }

  void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) override;
  void OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                       const CefString& url) override;
  void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
  void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;
  void OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool is_loading,
                            bool can_go_back, bool can_go_forward) override;

  CefRefPtr<CefResourceRequestHandler> GetResourceRequestHandler(
      CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
      CefRefPtr<CefRequest> request, bool is_navigation, bool is_download,
      const CefString& request_initiator, bool& disable_default_handling) override;
  cef_return_value_t OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request,
      CefRefPtr<CefCallback> callback) override;

  cef_runtime_style_t GetBrowserRuntimeStyle() override;
  void OnButtonPressed(CefRefPtr<CefButton> button) override;
  bool OnKeyEvent(CefRefPtr<CefTextfield> textfield, const CefKeyEvent& event) override;
  void OnWindowCreated(CefRefPtr<CefWindow> window) override;
  void OnWindowDestroyed(CefRefPtr<CefWindow> window) override;
  bool CanClose(CefRefPtr<CefWindow> window) override;
  CefSize GetPreferredSize(CefRefPtr<CefView> view) override;
  CefSize GetMinimumSize(CefRefPtr<CefView> view) override;
  cef_runtime_style_t GetWindowRuntimeStyle() override;

 private:
  explicit BrowserWindow(std::string startup_url);
  void LoadRules();
  void Navigate(const std::string& input);
  void UpdateNavigationState(bool is_loading, bool can_go_back, bool can_go_forward);
  void UpdateShieldLabel();
  static std::string ExecutableDirectory();

  std::string startup_url_;
  FilterEngine filter_engine_;
  PrivacyStats privacy_stats_;
  SiteShields site_shields_;
  CefRefPtr<CefBrowser> browser_;
  CefRefPtr<CefBrowserView> browser_view_;
  CefRefPtr<CefWindow> window_;
  CefRefPtr<CefTextfield> address_field_;
  CefRefPtr<CefLabelButton> shield_button_;

  IMPLEMENT_REFCOUNTING(BrowserWindow);
  DISALLOW_COPY_AND_ASSIGN(BrowserWindow);
};
}  // namespace bravelike
