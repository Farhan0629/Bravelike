#include "src/browser/browser_window.h"
#include "src/browser/home_url.h"
#include <filesystem>
#include <windows.h>
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_callback.h"
#include "include/cef_frame.h"
#include "include/cef_request.h"
#include "include/views/cef_box_layout.h"
#include "include/views/cef_panel.h"
#include "include/wrapper/cef_helpers.h"
#include "src/core/home_policy.h"
#include "src/core/navigation.h"
namespace kingfn {
namespace {
enum ControlId { kWindow = 1, kBack, kForward, kReload, kStop, kHome, kAddress, kShield, kBrowserView };
CefRefPtr<CefLabelButton> MakeButton(CefRefPtr<CefButtonDelegate> delegate, const char* label, int id) {
  auto button = CefLabelButton::CreateLabelButton(delegate, label);
  button->SetID(id); button->SetInkDropEnabled(true); button->SetFocusable(false); return button;
}
void SetEnabled(CefRefPtr<CefWindow> window, int id, bool enabled) {
  if (window) if (auto view = window->GetViewForID(id)) view->SetEnabled(enabled);
}
}
BrowserWindow::BrowserWindow(std::string startup_url)
    : startup_url_(std::move(startup_url)), home_url_(DefaultHomeUrl()) {
  LoadRules(); site_shields_.SetActiveUrl(startup_url_);
}
void BrowserWindow::Create(const std::string& startup_url) {
  CEF_REQUIRE_UI_THREAD();
  CefRefPtr<BrowserWindow> controller = new BrowserWindow(startup_url);
  CefBrowserSettings settings;
  controller->browser_view_ = CefBrowserView::CreateBrowserView(controller, startup_url, settings, nullptr, nullptr, controller);
  controller->browser_view_->SetID(kBrowserView); CefWindow::CreateTopLevelWindow(controller);
}
void BrowserWindow::LoadRules() {
  try { filter_engine_.LoadFromFile(ExecutableDirectory() + "\\config\\blocklist.txt"); }
  catch (...) { filter_engine_.LoadFromText("||doubleclick.net^\n||googlesyndication.com^\n||google-analytics.com^\n||facebook.net^\n"); }
}
std::string BrowserWindow::ExecutableDirectory() {
  wchar_t path[MAX_PATH]{};
  const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
  if (!length || length == MAX_PATH) return ".";
  return CefString(std::filesystem::path(path).parent_path().wstring()).ToString();
}
std::string BrowserWindow::DefaultHomeUrl() {
  const auto directory = std::filesystem::u8path(ExecutableDirectory());
  const auto path = std::filesystem::absolute(directory / "resources" / "home.html").lexically_normal();
  if (!std::filesystem::exists(path)) return "about:blank";
  return CanonicalHomeFileUrl(CefString(path.wstring())).ToString();
}
bool BrowserWindow::IsHomeUrl(const std::string& url) { return IsExactHomeUrl(url, DefaultHomeUrl()); }
void BrowserWindow::OnWindowCreated(CefRefPtr<CefWindow> window) {
  CEF_REQUIRE_UI_THREAD(); window_ = window; window_->SetID(kWindow); window_->SetTitle("KINGFN");
  CefBoxLayoutSettings root_settings; root_settings.horizontal = false; root_settings.between_child_spacing = 4;
  auto root_layout = window_->SetToBoxLayout(root_settings);
  auto toolbar = CefPanel::CreatePanel(nullptr);
  CefBoxLayoutSettings toolbar_settings; toolbar_settings.horizontal = true; toolbar_settings.between_child_spacing = 4;
  toolbar_settings.inside_border_insets = CefInsets(6, 8, 6, 8);
  auto layout = toolbar->SetToBoxLayout(toolbar_settings);
  toolbar->AddChildView(MakeButton(this, "Back", kBack));
  toolbar->AddChildView(MakeButton(this, "Forward", kForward));
  toolbar->AddChildView(MakeButton(this, "Reload", kReload));
  toolbar->AddChildView(MakeButton(this, "Stop", kStop));
  toolbar->AddChildView(MakeButton(this, "Home", kHome));
  address_field_ = CefTextfield::CreateTextfield(this); address_field_->SetID(kAddress);
  if (!IsExactHomeUrl(startup_url_, home_url_)) address_field_->SetText(startup_url_);
  address_field_->SetAccessibleName("Address and search bar");
  toolbar->AddChildView(address_field_); layout->SetFlexForView(address_field_, 1);
  shield_button_ = MakeButton(this, "Shields: On", kShield); toolbar->AddChildView(shield_button_);
  window_->AddChildView(toolbar); window_->AddChildView(browser_view_); root_layout->SetFlexForView(browser_view_, 1);
  window_->CenterWindow(CefSize(1280, 800)); UpdateNavigationState(true, false, false); UpdateShieldLabel(); window_->Show(); FocusAddressBar();
}
void BrowserWindow::OnWindowDestroyed(CefRefPtr<CefWindow> window) {
  CEF_REQUIRE_UI_THREAD(); address_field_ = nullptr; shield_button_ = nullptr; browser_view_ = nullptr; window_ = nullptr;
}
bool BrowserWindow::CanClose(CefRefPtr<CefWindow> window) { CEF_REQUIRE_UI_THREAD(); return !browser_ || browser_->GetHost()->TryCloseBrowser(); }
CefSize BrowserWindow::GetPreferredSize(CefRefPtr<CefView> view) { return view->GetID() == kWindow ? CefSize(1280, 800) : CefSize(); }
CefSize BrowserWindow::GetMinimumSize(CefRefPtr<CefView> view) { return view->GetID() == kWindow ? CefSize(720, 480) : CefSize(); }
cef_runtime_style_t BrowserWindow::GetWindowRuntimeStyle() { return CEF_RUNTIME_STYLE_ALLOY; }
cef_runtime_style_t BrowserWindow::GetBrowserRuntimeStyle() { return CEF_RUNTIME_STYLE_ALLOY; }
void BrowserWindow::OnAfterCreated(CefRefPtr<CefBrowser> browser) { CEF_REQUIRE_UI_THREAD(); browser_ = browser; }
void BrowserWindow::OnBeforeClose(CefRefPtr<CefBrowser> browser) { CEF_REQUIRE_UI_THREAD(); browser_ = nullptr; CefQuitMessageLoop(); }
void BrowserWindow::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) {
  CEF_REQUIRE_UI_THREAD();
  if (window_) window_->SetTitle(title.empty() || title.ToString() == "KINGFN" ? "KINGFN" : title.ToString() + " - KINGFN");
}
void BrowserWindow::OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& url) {
  CEF_REQUIRE_UI_THREAD(); if (!frame->IsMain()) return;
  site_shields_.SetActiveUrl(url.ToString());
  if (address_field_) address_field_->SetText(IsExactHomeUrl(url.ToString(), home_url_) ? CefString("") : url);
  UpdateShieldLabel();
}
void BrowserWindow::OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool loading, bool back, bool forward) {
  CEF_REQUIRE_UI_THREAD(); UpdateNavigationState(loading, back, forward); UpdateShieldLabel();
}
bool BrowserWindow::OnPreKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent& event, CefEventHandle os_event, bool* shortcut) {
  CEF_REQUIRE_UI_THREAD(); if (event.type != KEYEVENT_RAWKEYDOWN || !browser_) return false;
  const bool control = (event.modifiers & EVENTFLAG_CONTROL_DOWN) != 0;
  const bool alt = (event.modifiers & EVENTFLAG_ALT_DOWN) != 0;
  const bool shift = (event.modifiers & EVENTFLAG_SHIFT_DOWN) != 0;
  if (shift || (control && alt)) return false;
  if (control && event.windows_key_code == 'L') { FocusAddressBar(); return true; }
  if (control && event.windows_key_code == 'R') { browser_->Reload(); return true; }
  if (alt && event.windows_key_code == VK_LEFT && browser_->CanGoBack()) { browser_->GoBack(); return true; }
  if (alt && event.windows_key_code == VK_RIGHT && browser_->CanGoForward()) { browser_->GoForward(); return true; }
  if (alt && event.windows_key_code == VK_HOME) { browser_->GetMainFrame()->LoadURL(home_url_); return true; }
  if (!control && !alt && event.windows_key_code == VK_ESCAPE && browser_->IsLoading()) { browser_->StopLoad(); return true; }
  return false;
}
void BrowserWindow::UpdateNavigationState(bool loading, bool back, bool forward) {
  SetEnabled(window_, kBack, back); SetEnabled(window_, kForward, forward); SetEnabled(window_, kReload, !loading);
  SetEnabled(window_, kStop, loading); SetEnabled(window_, kHome, true); SetEnabled(window_, kShield, site_shields_.ActiveHost().has_value());
}
void BrowserWindow::UpdateShieldLabel() {
  if (!shield_button_) return;
  const auto host = site_shields_.ActiveHost(); shield_button_->SetEnabled(host.has_value());
  if (!host) { shield_button_->SetText("Shields: N/A"); return; }
  shield_button_->SetText(site_shields_.EnabledForActive() ? "Shields: On (" + std::to_string(privacy_stats_.Snapshot().blocked) + ")" : "Shields: Off");
}
void BrowserWindow::Navigate(const std::string& input) {
  if (!browser_) return;
  if (IsHomeAlias(input)) { browser_->GetMainFrame()->LoadURL(home_url_); return; }
  const auto url = ResolveAddressInput(input); if (!url.empty()) browser_->GetMainFrame()->LoadURL(url);
}
void BrowserWindow::FocusAddressBar() { if (address_field_) { address_field_->RequestFocus(); address_field_->SelectAll(false); } }
void BrowserWindow::OnButtonPressed(CefRefPtr<CefButton> button) {
  CEF_REQUIRE_UI_THREAD(); if (!browser_) return;
  switch (button->GetID()) {
    case kBack: if (browser_->CanGoBack()) browser_->GoBack(); break;
    case kForward: if (browser_->CanGoForward()) browser_->GoForward(); break;
    case kReload: browser_->Reload(); break;
    case kStop: browser_->StopLoad(); break;
    case kHome: browser_->GetMainFrame()->LoadURL(home_url_); break;
    case kShield: if (site_shields_.ToggleActive()) { UpdateShieldLabel(); browser_->Reload(); } break;
    default: break;
  }
}
bool BrowserWindow::OnKeyEvent(CefRefPtr<CefTextfield> field, const CefKeyEvent& event) {
  CEF_REQUIRE_UI_THREAD(); if (field->GetID() != kAddress) return false;
  if (event.type == KEYEVENT_RAWKEYDOWN && event.windows_key_code == VK_RETURN) { Navigate(field->GetText().ToString()); return true; }
  bool shortcut = false; return OnPreKeyEvent(browser_, event, nullptr, &shortcut);
}
CefRefPtr<CefResourceRequestHandler> BrowserWindow::GetResourceRequestHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
    CefRefPtr<CefRequest> request, bool navigation, bool download, const CefString& initiator, bool& disable) { return this; }
cef_return_value_t BrowserWindow::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
    CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
  const bool enabled = site_shields_.EnabledForActive();
  const auto decision = enabled ? filter_engine_.Evaluate(request->GetURL().ToString()) : FilterDecision{};
  const bool blocked = enabled && decision.action == FilterAction::kBlock; privacy_stats_.Record(blocked);
  return blocked ? RV_CANCEL : RV_CONTINUE;
}
}
