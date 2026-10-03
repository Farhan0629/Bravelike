#pragma once
#include <memory>
#include <string>
#include <vector>

#include "include/cef_client.h"
#include "include/cef_keyboard_handler.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_browser_view_delegate.h"
#include "include/views/cef_button_delegate.h"
#include "include/views/cef_label_button.h"
#include "include/views/cef_panel.h"
#include "include/views/cef_textfield.h"
#include "include/views/cef_textfield_delegate.h"
#include "include/views/cef_window.h"
#include "include/views/cef_window_delegate.h"

#include "src/core/database.h"
#include "src/core/filter_engine.h"
#include "src/core/privacy_stats.h"
#include "src/core/site_shields.h"
#include "src/browser/download_handler.h"
#include "src/browser/update_checker.h"

namespace kingfn {

// ── Per-tab state ─────────────────────────────────────────────────────────────
struct TabEntry {
  int                         tab_id{-1};
  CefRefPtr<CefBrowserView>   browser_view;
  CefRefPtr<CefBrowser>       browser;
  std::string                 url;
  std::string                 title{"New Tab"};
  bool                        is_loading{false};
  bool                        can_go_back{false};
  bool                        can_go_forward{false};
  SiteShields                 shields;   // per-tab shields
  PrivacyStats                stats;    // per-tab blocked counter
};

// ── ID ranges for CEF Views controls ─────────────────────────────────────────
// kWindow=1, kBack=2, kForward=3, kReload=4, kStop=5, kHome=6
// kAddress=7, kShield=8, kBookmark=9, kDownloads=10
// kNewTab=11
// kTabButton[i]  = kTabButtonBase + i   (switch to tab i)
// kTabClose[i]   = kTabCloseBase  + i   (close tab i)
constexpr int kTabButtonBase = 100;
constexpr int kTabCloseBase  = 300;
constexpr int kBrowserViewBase = 500;  // browser view IDs

class BrowserWindow final
    : public CefClient,
      public CefDisplayHandler,
      public CefLifeSpanHandler,
      public CefLoadHandler,
      public CefRequestHandler,
      public CefResourceRequestHandler,
      public CefKeyboardHandler,
      public CefBrowserViewDelegate,
      public CefButtonDelegate,
      public CefTextfieldDelegate,
      public CefWindowDelegate {
 public:
  static void Create(const std::string& startup_url);
  static std::string DefaultHomeUrl();
  static bool IsHomeUrl(const std::string& url);
  static bool IsInternalUrl(const std::string& url);

  // ── Handler getters ───────────────────────────────────────────────────────
  CefRefPtr<CefDisplayHandler>  GetDisplayHandler()  override { return this; }
  CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
  CefRefPtr<CefLoadHandler>     GetLoadHandler()     override { return this; }
  CefRefPtr<CefRequestHandler>  GetRequestHandler()  override { return this; }
  CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() override { return this; }
  CefRefPtr<CefDownloadHandler> GetDownloadHandler() override {
    return download_handler_;
  }

  // ── CefDisplayHandler ─────────────────────────────────────────────────────
  void OnTitleChange(CefRefPtr<CefBrowser>, const CefString&) override;
  void OnAddressChange(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>,
                       const CefString&) override;
  void OnFullscreenModeChange(CefRefPtr<CefBrowser>, bool) override;
  bool OnConsoleMessage(CefRefPtr<CefBrowser>, cef_log_severity_t,
                        const CefString&, const CefString&, int) override;

  // ── CefLifeSpanHandler ────────────────────────────────────────────────────
  void OnAfterCreated(CefRefPtr<CefBrowser>) override;
  void OnBeforeClose(CefRefPtr<CefBrowser>) override;
  bool OnBeforePopup(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>,
                     const CefString& target_url,
                     const CefString& target_frame_name,
                     WindowOpenDisposition, bool user_gesture,
                     const CefPopupFeatures&, CefWindowInfo&,
                     CefRefPtr<CefClient>&, CefBrowserSettings&,
                     CefRefPtr<CefDictionaryValue>&,
                     bool* no_javascript_access) override;

  // ── CefLoadHandler ────────────────────────────────────────────────────────
  void OnLoadingStateChange(CefRefPtr<CefBrowser>, bool, bool, bool) override;
  void OnLoadStart(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>,
                   TransitionType) override;
  void OnLoadEnd(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, int) override;
  void OnLoadError(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>,
                   ErrorCode, const CefString&, const CefString&) override;

  // ── CefKeyboardHandler ────────────────────────────────────────────────────
  bool OnPreKeyEvent(CefRefPtr<CefBrowser>, const CefKeyEvent&,
                     CefEventHandle, bool*) override;

  // ── CefRequestHandler / CefResourceRequestHandler ─────────────────────────
  CefRefPtr<CefResourceRequestHandler> GetResourceRequestHandler(
      CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, CefRefPtr<CefRequest>,
      bool, bool, const CefString&, bool&) override;
  cef_return_value_t OnBeforeResourceLoad(
      CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, CefRefPtr<CefRequest>,
      CefRefPtr<CefCallback>) override;

  // ── View delegates ────────────────────────────────────────────────────────
  cef_runtime_style_t GetBrowserRuntimeStyle() override;
  void OnButtonPressed(CefRefPtr<CefButton>) override;
  bool OnKeyEvent(CefRefPtr<CefTextfield>, const CefKeyEvent&) override;

  // ── CefWindowDelegate ─────────────────────────────────────────────────────
  void    OnWindowCreated(CefRefPtr<CefWindow>) override;
  void    OnWindowDestroyed(CefRefPtr<CefWindow>) override;
  bool    CanClose(CefRefPtr<CefWindow>) override;
  CefSize GetPreferredSize(CefRefPtr<CefView>) override;
  CefSize GetMinimumSize(CefRefPtr<CefView>) override;
  cef_runtime_style_t GetWindowRuntimeStyle() override;

 private:
  explicit BrowserWindow(std::string startup_url);

  // ── Initialization ────────────────────────────────────────────────────────
  void LoadRules();
  static std::string ExecutableDirectory();

  // ── Tab management ────────────────────────────────────────────────────────
  int  NewTab(const std::string& url = "");
  void CloseTab(int tab_index);
  void ActivateTab(int tab_index);
  int  FindTabByBrowserId(int browser_id) const;
  TabEntry* ActiveTab();
  const TabEntry* ActiveTab() const;

  // ── UI helpers ────────────────────────────────────────────────────────────
  void RebuildTabStrip();
  void UpdateToolbar();
  void UpdateShieldLabel();
  void UpdateBookmarkButton();
  void UpdateDownloadButton(int active_count);
  void InjectShieldsScript(CefRefPtr<CefFrame> frame, TabEntry* tab);
  void InjectPageData(CefRefPtr<CefFrame> frame, const std::string& url);
  void Navigate(const std::string& input);
  void FocusAddressBar();
  void ToggleBookmark();
  void OnUpdateAvailable(std::string new_ver, std::string url);

  // ── State ─────────────────────────────────────────────────────────────────
  std::string startup_url_;
  std::string shields_script_;
  std::string history_html_;
  std::string downloads_html_;
  int next_tab_id_{0};
  int active_tab_index_{-1};
  bool is_fullscreen_{false};
  int closing_tab_count_{0};  // browsers still waiting for OnBeforeClose

  std::vector<TabEntry> tabs_;

  // Core engine (shared across all tabs).
  FilterEngine filter_engine_;

  // Database (history, bookmarks, shields, downloads).
  std::unique_ptr<Database> db_;

  // Download handler.
  CefRefPtr<DownloadHandler> download_handler_;

  // Update checker.
  std::unique_ptr<UpdateChecker> update_checker_;
  CefRefPtr<CefLabelButton>  update_button_;

  // CEF Views references.
  CefRefPtr<CefWindow>       window_;
  CefRefPtr<CefPanel>        tab_strip_;
  CefRefPtr<CefTextfield>    address_field_;
  CefRefPtr<CefLabelButton>  shield_button_;
  CefRefPtr<CefLabelButton>  bookmark_button_;
  CefRefPtr<CefLabelButton>  download_button_;
  CefRefPtr<CefPanel>        content_panel_;

  IMPLEMENT_REFCOUNTING(BrowserWindow);
  DISALLOW_COPY_AND_ASSIGN(BrowserWindow);
};

}  // namespace kingfn
