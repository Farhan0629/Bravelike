#include "src/browser/browser_window.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <windows.h>

#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_callback.h"
#include "include/cef_frame.h"
#include "include/cef_request.h"
#include "include/views/cef_box_layout.h"
#include "include/views/cef_panel.h"
#include "include/wrapper/cef_helpers.h"
#include "src/core/navigation.h"

namespace kingfn {
namespace {
enum ControlId {
  kWindow = 1,
  kBack,
  kForward,
  kReload,
  kStop,
  kHome,
  kAddress,
  kShield,
  kBrowserView,
};

CefRefPtr<CefLabelButton> MakeButton(CefRefPtr<CefButtonDelegate> delegate,
                                     const char* label, int id) {
  auto button = CefLabelButton::CreateLabelButton(delegate, label);
  button->SetID(id);
  button->SetInkDropEnabled(true);
  button->SetFocusable(false);
  return button;
}

void SetEnabled(CefRefPtr<CefWindow> window, int id, bool enabled) {
  if (window) {
    if (auto view = window->GetViewForID(id)) view->SetEnabled(enabled);
  }
}
}  // namespace

BrowserWindow::BrowserWindow(std::string startup_url)
    : startup_url_(std::move(startup_url)) {
  LoadRules();
  site_shields_.SetActiveUrl(startup_url_);
}

void BrowserWindow::Create(const std::string& startup_url) {
  CEF_REQUIRE_UI_THREAD();
  CefRefPtr<BrowserWindow> controller = new BrowserWindow(startup_url);
  CefBrowserSettings settings;
  controller->browser_view_ = CefBrowserView::CreateBrowserView(
      controller, startup_url, settings, nullptr, nullptr, controller);
  controller->browser_view_->SetID(kBrowserView);
  CefWindow::CreateTopLevelWindow(controller);
}

namespace {
std::string ReadTextFile(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) return {};
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

constexpr const char* kFallbackRules =
    "doubleclick.net\ngooglesyndication.com\ngoogleadservices.com\n"
    "google-analytics.com\ngoogletagmanager.com\nadservice.google.com\n"
    "amazon-adsystem.com\nadnxs.com\npubmatic.com\nrubiconproject.com\n"
    "criteo.com\ntaboola.com\noutbrain.com\nfacebook.net\n"
    "||youtube.com/api/stats/ads\n||youtube.com/pagead/\n"
    "||youtube.com/ptracking\n||youtube.com/get_midroll_info\n";
}  // namespace

void BrowserWindow::LoadRules() {
  const auto base = std::filesystem::path(ExecutableDirectory());
  const std::string rules = ReadTextFile(base / "config" / "blocklist.txt");
  filter_engine_.LoadFromText(rules.empty() ? kFallbackRules : rules);
  // Optional user rules, appended on top of the defaults.
  const std::string custom = ReadTextFile(base / "config" / "custom-blocklist.txt");
  if (!custom.empty()) filter_engine_.AppendFromText(custom);

  shields_script_ = ReadTextFile(base / "resources" / "shields.js");
}

void BrowserWindow::InjectShieldsScript(CefRefPtr<CefFrame> frame) {
  if (shields_script_.empty() || !frame || !frame->IsMain()) return;
  const std::string url = frame->GetURL().ToString();
  if (url.rfind("http://", 0) != 0 && url.rfind("https://", 0) != 0) return;
  if (!site_shields_.EnabledForActive()) return;
  frame->ExecuteJavaScript(shields_script_, url, 0);
}

void BrowserWindow::OnLoadStart(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                TransitionType transition_type) {
  CEF_REQUIRE_UI_THREAD();
  InjectShieldsScript(frame);
}

void BrowserWindow::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame, int http_status_code) {
  CEF_REQUIRE_UI_THREAD();
  // The script guards against double execution on full page loads, so
  // re-injecting is safe and covers documents where OnLoadStart ran too early.
  InjectShieldsScript(frame);
}

std::string BrowserWindow::ExecutableDirectory() {
  wchar_t path[MAX_PATH]{};
  const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
  if (length == 0 || length == MAX_PATH) return ".";
  return std::filesystem::path(path).parent_path().string();
}

std::string BrowserWindow::DefaultHomeUrl() {
  const auto exe_dir = std::filesystem::path(ExecutableDirectory());
  const auto home_path = exe_dir / "resources" / "home.html";
  if (std::filesystem::exists(home_path)) {
    std::string path_str = std::filesystem::absolute(home_path).lexically_normal().string();
    if (path_str.rfind("\\\\\\.\\?\\\\", 0) == 0) {
      path_str = path_str.substr(4);
    }
    for (char& c : path_str) {
      if (c == '\\') c = '/';
    }
    return "file:///" + path_str;
  }
  return "about:blank";
}

bool BrowserWindow::IsHomeUrl(const std::string& url) {
  if (url.empty() || url == "about:blank" || url == "about:home") return true;
  if (url.find("resources/home.html") != std::string::npos ||
      url.find("resources\\home.html") != std::string::npos) {
    return true;
  }
  return false;
}

void BrowserWindow::OnWindowCreated(CefRefPtr<CefWindow> window) {
  CEF_REQUIRE_UI_THREAD();
  window_ = window;
  window_->SetID(kWindow);
  window_->SetTitle("KINGFN");

  CefBoxLayoutSettings root_settings;
  root_settings.horizontal = false;
  root_settings.between_child_spacing = 4;
  auto root_layout = window_->SetToBoxLayout(root_settings);

  auto toolbar = CefPanel::CreatePanel(nullptr);
  CefBoxLayoutSettings toolbar_settings;
  toolbar_settings.horizontal = true;
  toolbar_settings.between_child_spacing = 4;
  toolbar_settings.inside_border_insets = CefInsets(6, 8, 6, 8);
  auto toolbar_layout = toolbar->SetToBoxLayout(toolbar_settings);

  toolbar->AddChildView(MakeButton(this, "Back", kBack));
  toolbar->AddChildView(MakeButton(this, "Forward", kForward));
  toolbar->AddChildView(MakeButton(this, "Reload", kReload));
  toolbar->AddChildView(MakeButton(this, "Stop", kStop));
  toolbar->AddChildView(MakeButton(this, "Home", kHome));

  address_field_ = CefTextfield::CreateTextfield(this);
  address_field_->SetID(kAddress);
  if (!IsHomeUrl(startup_url_)) {
    address_field_->SetText(startup_url_);
  }
  address_field_->SetAccessibleName("Address and search bar");
  toolbar->AddChildView(address_field_);
  toolbar_layout->SetFlexForView(address_field_, 1);

  shield_button_ = MakeButton(this, "Shields: On", kShield);
  toolbar->AddChildView(shield_button_);

  window_->AddChildView(toolbar);
  window_->AddChildView(browser_view_);
  root_layout->SetFlexForView(browser_view_, 1);
  window_->CenterWindow(CefSize(1280, 800));
  UpdateNavigationState(true, false, false);
  UpdateShieldLabel();
  window_->Show();
  FocusAddressBar();
}

void BrowserWindow::OnWindowDestroyed(CefRefPtr<CefWindow> window) {
  CEF_REQUIRE_UI_THREAD();
  address_field_ = nullptr;
  shield_button_ = nullptr;
  browser_view_ = nullptr;
  window_ = nullptr;
}

bool BrowserWindow::CanClose(CefRefPtr<CefWindow> window) {
  CEF_REQUIRE_UI_THREAD();
  return !browser_ || browser_->GetHost()->TryCloseBrowser();
}

CefSize BrowserWindow::GetPreferredSize(CefRefPtr<CefView> view) {
  if (view->GetID() == kWindow) return CefSize(1280, 800);
  return CefSize();
}

CefSize BrowserWindow::GetMinimumSize(CefRefPtr<CefView> view) {
  if (view->GetID() == kWindow) return CefSize(720, 480);
  return CefSize();
}

cef_runtime_style_t BrowserWindow::GetWindowRuntimeStyle() {
  return CEF_RUNTIME_STYLE_ALLOY;
}

cef_runtime_style_t BrowserWindow::GetBrowserRuntimeStyle() {
  return CEF_RUNTIME_STYLE_ALLOY;
}

void BrowserWindow::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  browser_ = browser;
}

void BrowserWindow::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  browser_ = nullptr;
  CefQuitMessageLoop();
}

void BrowserWindow::OnTitleChange(CefRefPtr<CefBrowser> browser,
                                  const CefString& title) {
  CEF_REQUIRE_UI_THREAD();
  if (window_) {
    const std::string title_str = title.ToString();
    if (title_str.empty() || title_str == "KINGFN") {
      window_->SetTitle("KINGFN");
    } else {
      window_->SetTitle(title_str + " - KINGFN");
    }
  }
}

void BrowserWindow::OnAddressChange(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    const CefString& url) {
  CEF_REQUIRE_UI_THREAD();
  if (!frame->IsMain()) return;
  const std::string url_str = url.ToString();
  site_shields_.SetActiveUrl(url_str);
  if (address_field_) {
    if (IsHomeUrl(url_str)) {
      address_field_->SetText("");
    } else {
      address_field_->SetText(url);
    }
  }
  UpdateShieldLabel();

  // SPA navigation (e.g. YouTube video-to-video): OnLoadStart/End may not
  // fire again. Re-inject the shields script whenever the address changes
  // so the YouTube ad killer stays active for the new page context.
  if (browser_ && frame->IsMain()) {
    InjectShieldsScript(frame);
  }
}

void BrowserWindow::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                         bool is_loading, bool can_go_back,
                                         bool can_go_forward) {
  CEF_REQUIRE_UI_THREAD();
  UpdateNavigationState(is_loading, can_go_back, can_go_forward);
  UpdateShieldLabel();
}

bool BrowserWindow::OnPreKeyEvent(CefRefPtr<CefBrowser> browser,
                                  const CefKeyEvent& event,
                                  CefEventHandle os_event,
                                  bool* is_keyboard_shortcut) {
  CEF_REQUIRE_UI_THREAD();
  if (event.type != KEYEVENT_RAWKEYDOWN || !browser_) return false;

  const bool control = (event.modifiers & EVENTFLAG_CONTROL_DOWN) != 0;
  const bool alt = (event.modifiers & EVENTFLAG_ALT_DOWN) != 0;
  switch (event.windows_key_code) {
    case 'L':
      if (control) {
        FocusAddressBar();
        return true;
      }
      break;
    case 'R':
      if (control) {
        browser_->Reload();
        return true;
      }
      break;
    case VK_LEFT:
      if (alt && browser_->CanGoBack()) {
        browser_->GoBack();
        return true;
      }
      break;
    case VK_RIGHT:
      if (alt && browser_->CanGoForward()) {
        browser_->GoForward();
        return true;
      }
      break;
    case VK_HOME:
      if (alt) {
        browser_->GetMainFrame()->LoadURL(startup_url_);
        return true;
      }
      break;
    case VK_ESCAPE:
      browser_->StopLoad();
      return true;
    default:
      break;
  }
  return false;
}

void BrowserWindow::UpdateNavigationState(bool is_loading, bool can_go_back,
                                          bool can_go_forward) {
  SetEnabled(window_, kBack, can_go_back);
  SetEnabled(window_, kForward, can_go_forward);
  SetEnabled(window_, kReload, !is_loading);
  SetEnabled(window_, kStop, is_loading);
  SetEnabled(window_, kHome, true);
  SetEnabled(window_, kShield, site_shields_.ActiveHost().has_value());
}

void BrowserWindow::UpdateShieldLabel() {
  if (!shield_button_) return;
  const auto host = site_shields_.ActiveHost();
  shield_button_->SetEnabled(host.has_value());
  if (!host) {
    shield_button_->SetText("Shields: N/A");
    return;
  }
  const auto snapshot = privacy_stats_.Snapshot();
  std::string label = site_shields_.EnabledForActive()
      ? "Shields: On (" + std::to_string(snapshot.blocked) + ")"
      : "Shields: Off";
  shield_button_->SetText(label);
}

void BrowserWindow::Navigate(const std::string& input) {
  if (!browser_) return;
  if (input == "home" || input == "kingfn" || input == "kingfn://home" ||
      input == "kingfn://newtab" || input == "about:home") {
    browser_->GetMainFrame()->LoadURL(startup_url_);
    return;
  }
  const auto url = ResolveAddressInput(input);
  if (!url.empty()) browser_->GetMainFrame()->LoadURL(url);
}

void BrowserWindow::FocusAddressBar() {
  if (!address_field_) return;
  address_field_->RequestFocus();
  address_field_->SelectAll(false);
}

void BrowserWindow::OnButtonPressed(CefRefPtr<CefButton> button) {
  CEF_REQUIRE_UI_THREAD();
  if (!browser_) return;
  switch (button->GetID()) {
    case kBack: browser_->GoBack(); break;
    case kForward: browser_->GoForward(); break;
    case kReload: browser_->Reload(); break;
    case kStop: browser_->StopLoad(); break;
    case kHome: browser_->GetMainFrame()->LoadURL(startup_url_); break;
    case kShield:
      if (site_shields_.ToggleActive()) {
        UpdateShieldLabel();
        browser_->Reload();
      }
      break;
    default: break;
  }
}

bool BrowserWindow::OnKeyEvent(CefRefPtr<CefTextfield> textfield,
                               const CefKeyEvent& event) {
  CEF_REQUIRE_UI_THREAD();
  if (textfield->GetID() == kAddress && event.type == KEYEVENT_RAWKEYDOWN &&
      event.windows_key_code == VK_RETURN) {
    Navigate(textfield->GetText().ToString());
    return true;
  }
  return false;
}

CefRefPtr<CefResourceRequestHandler> BrowserWindow::GetResourceRequestHandler(
    CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
    CefRefPtr<CefRequest> request, bool is_navigation, bool is_download,
    const CefString& request_initiator, bool& disable_default_handling) {
  return this;
}

cef_return_value_t BrowserWindow::OnBeforeResourceLoad(
    CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
    CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
  const bool enabled = site_shields_.EnabledForActive();
  const auto decision = enabled ? filter_engine_.Evaluate(request->GetURL().ToString())
                                : FilterDecision{};
  const bool blocked = enabled && decision.action == FilterAction::kBlock;
  privacy_stats_.Record(blocked);
  return blocked ? RV_CANCEL : RV_CONTINUE;
}
}  // namespace kingfn
