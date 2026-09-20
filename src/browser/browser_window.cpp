#include "src/browser/browser_window.h"

#include <filesystem>
#include <windows.h>

#include "include/cef_browser.h"
#include "include/cef_callback.h"
#include "include/cef_frame.h"
#include "include/cef_request.h"
#include "include/views/cef_box_layout.h"
#include "include/views/cef_panel.h"
#include "include/wrapper/cef_helpers.h"
#include "src/core/navigation.h"

namespace bravelike {
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

void BrowserWindow::LoadRules() {
  try {
    filter_engine_.LoadFromFile(ExecutableDirectory() + "\\config\\blocklist.txt");
  } catch (...) {
    filter_engine_.LoadFromText(
        "||doubleclick.net^\n||googlesyndication.com^\n"
        "||google-analytics.com^\n||facebook.net^\n");
  }
}

std::string BrowserWindow::ExecutableDirectory() {
  wchar_t path[MAX_PATH]{};
  const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
  if (length == 0 || length == MAX_PATH) return ".";
  return std::filesystem::path(path).parent_path().string();
}

void BrowserWindow::OnWindowCreated(CefRefPtr<CefWindow> window) {
  CEF_REQUIRE_UI_THREAD();
  window_ = window;
  window_->SetID(kWindow);
  window_->SetTitle("Bravelike");

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
  address_field_->SetText(startup_url_);
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
  window_->Show();
  address_field_->RequestFocus();
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
  if (window_) window_->SetTitle(title.empty() ? "Bravelike" : title.ToString() + " - Bravelike");
}

void BrowserWindow::OnAddressChange(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    const CefString& url) {
  CEF_REQUIRE_UI_THREAD();
  if (frame->IsMain() && address_field_) address_field_->SetText(url);
}

void BrowserWindow::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                         bool is_loading, bool can_go_back,
                                         bool can_go_forward) {
  CEF_REQUIRE_UI_THREAD();
  UpdateNavigationState(is_loading, can_go_back, can_go_forward);
  UpdateShieldLabel();
}

void BrowserWindow::UpdateNavigationState(bool is_loading, bool can_go_back,
                                          bool can_go_forward) {
  SetEnabled(window_, kBack, can_go_back);
  SetEnabled(window_, kForward, can_go_forward);
  SetEnabled(window_, kReload, !is_loading);
  SetEnabled(window_, kStop, is_loading);
  SetEnabled(window_, kHome, true);
  SetEnabled(window_, kShield, true);
}

void BrowserWindow::UpdateShieldLabel() {
  if (!shield_button_) return;
  const auto snapshot = privacy_stats_.Snapshot();
  std::string label = shields_enabled_.load(std::memory_order_relaxed)
      ? "Shields: On (" + std::to_string(snapshot.blocked) + ")"
      : "Shields: Off";
  shield_button_->SetText(label);
}

void BrowserWindow::Navigate(const std::string& input) {
  if (!browser_) return;
  const auto url = ResolveAddressInput(input);
  if (!url.empty()) browser_->GetMainFrame()->LoadURL(url);
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
      shields_enabled_.store(!shields_enabled_.load(std::memory_order_relaxed),
                             std::memory_order_relaxed);
      UpdateShieldLabel();
      browser_->Reload();
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
  const bool enabled = shields_enabled_.load(std::memory_order_relaxed);
  const auto decision = enabled ? filter_engine_.Evaluate(request->GetURL().ToString())
                                : FilterDecision{};
  const bool blocked = enabled && decision.action == FilterAction::kBlock;
  privacy_stats_.Record(blocked);
  return blocked ? RV_CANCEL : RV_CONTINUE;
}
}  // namespace bravelike
