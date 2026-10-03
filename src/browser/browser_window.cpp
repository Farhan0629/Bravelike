#include "src/browser/browser_window.h"

#include <algorithm>
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
#include "include/views/cef_fill_layout.h"
#include "include/views/cef_panel.h"
#include "include/wrapper/cef_helpers.h"
#include "src/core/navigation.h"

namespace kingfn {

// ── Control IDs (non-tab) ─────────────────────────────────────────────────────
enum ControlId {
  kWindow    = 1,
  kBack      = 2,
  kForward   = 3,
  kReload    = 4,
  kStop      = 5,
  kHome      = 6,
  kAddress   = 7,
  kShield    = 8,
  kBookmark  = 9,
  kDownloads = 10,
  kNewTab    = 11,
};

namespace {

std::string ReadTextFile(const std::filesystem::path& path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return {};
  std::ostringstream buf;
  buf << f.rdbuf();
  return buf.str();
}

CefRefPtr<CefLabelButton> MakeBtn(CefRefPtr<CefButtonDelegate> d,
                                   const char* label, int id,
                                   bool focusable = false) {
  auto btn = CefLabelButton::CreateLabelButton(d, label);
  btn->SetID(id);
  btn->SetInkDropEnabled(true);
  btn->SetFocusable(focusable);
  return btn;
}

void SetEnabled(CefRefPtr<CefWindow> w, int id, bool en) {
  if (w) if (auto v = w->GetViewForID(id)) v->SetEnabled(en);
}

// JSON-escape a string value.
std::string JE(const std::string& s) {
  std::string r;
  r.reserve(s.size() + 4);
  r += '"';
  for (unsigned char c : s) {
    if      (c == '"')  r += "\\\"";
    else if (c == '\\') r += "\\\\";
    else if (c == '\n') r += "\\n";
    else if (c == '\r') r += "\\r";
    else if (c == '\t') r += "\\t";
    else                r += static_cast<char>(c);
  }
  r += '"';
  return r;
}

constexpr const char* kFallbackRules =
    "doubleclick.net\ngooglesyndication.com\ngoogleadservices.com\n"
    "google-analytics.com\ngoogletagmanager.com\nadservice.google.com\n"
    "amazon-adsystem.com\nadnxs.com\npubmatic.com\nrubiconproject.com\n"
    "criteo.com\ntaboola.com\noutbrain.com\nfacebook.net\n"
    "||youtube.com/api/stats/ads\n||youtube.com/pagead/\n"
    "||youtube.com/ptracking\n||youtube.com/get_midroll_info\n"
    "||youtube.com/youtubei/v1/ad_break\n";

}  // namespace

// ── Construction ──────────────────────────────────────────────────────────────

BrowserWindow::BrowserWindow(std::string startup_url)
    : startup_url_(std::move(startup_url)) {
  LoadRules();
}

void BrowserWindow::Create(const std::string& startup_url) {
  CEF_REQUIRE_UI_THREAD();
  CefRefPtr<BrowserWindow> w = new BrowserWindow(startup_url);
  CefWindow::CreateTopLevelWindow(w);
}

std::string BrowserWindow::ExecutableDirectory() {
  wchar_t path[MAX_PATH]{};
  GetModuleFileNameW(nullptr, path, MAX_PATH);
  return std::filesystem::path(path).parent_path().string();
}

void BrowserWindow::LoadRules() {
  const auto base = std::filesystem::path(ExecutableDirectory());
  std::string rules = ReadTextFile(base / "config" / "kingfn-blocklist.txt");
  if (rules.empty()) rules = ReadTextFile(base / "config" / "blocklist.txt");
  filter_engine_.LoadFromText(rules.empty() ? kFallbackRules : rules);
  const std::string custom = ReadTextFile(base / "config" / "custom-blocklist.txt");
  if (!custom.empty()) filter_engine_.AppendFromText(custom);

  shields_script_ = ReadTextFile(base / "resources" / "shields.js");
  history_html_   = (base / "resources" / "history.html").string();
  downloads_html_ = (base / "resources" / "downloads.html").string();

  // Open database.
  const std::string db_path = (base / "KINGFNProfile" / "kingfn.db").string();
  std::filesystem::create_directories(base / "KINGFNProfile");
  db_ = std::make_unique<Database>(db_path);
}

// ── Static helpers ────────────────────────────────────────────────────────────

std::string BrowserWindow::DefaultHomeUrl() {
  const auto exe = std::filesystem::path(ExecutableDirectory());
  const auto home = exe / "resources" / "home.html";
  if (std::filesystem::exists(home)) {
    std::string p = std::filesystem::absolute(home).lexically_normal().string();
    if (p.size() >= 4 && p[0] == '\\' && p[1] == '\\' &&
        p[2] == '?' && p[3] == '\\')
      p = p.substr(4);
    for (char& c : p) if (c == '\\') c = '/';
    return "file:///" + p;
  }
  return "about:blank";
}

bool BrowserWindow::IsHomeUrl(const std::string& url) {
  if (url.empty() || url == "about:blank" || url == "about:home") return true;
  return url.find("resources/home.html") != std::string::npos ||
         url.find("resources\\home.html") != std::string::npos;
}

bool BrowserWindow::IsInternalUrl(const std::string& url) {
  return url.find("resources/history.html")  != std::string::npos ||
         url.find("resources/downloads.html") != std::string::npos ||
         IsHomeUrl(url);
}

// ── Window creation ───────────────────────────────────────────────────────────

void BrowserWindow::OnWindowCreated(CefRefPtr<CefWindow> window) {
  CEF_REQUIRE_UI_THREAD();
  window_ = window;
  window_->SetID(kWindow);
  window_->SetTitle("KINGFN");

  // Root: vertical box [tab_strip | toolbar | content_panel]
  CefBoxLayoutSettings root_box;
  root_box.horizontal = false;
  root_box.between_child_spacing = 0;
  auto root_layout = window_->SetToBoxLayout(root_box);

  // ── Tab strip ────────────────────────────────────────────────────────────
  tab_strip_ = CefPanel::CreatePanel(nullptr);
  CefBoxLayoutSettings tab_strip_box;
  tab_strip_box.horizontal = true;
  tab_strip_box.between_child_spacing = 2;
  tab_strip_box.inside_border_insets = CefInsets(4, 4, 0, 4);
  tab_strip_->SetToBoxLayout(tab_strip_box);
  window_->AddChildView(tab_strip_);

  // ── Toolbar ──────────────────────────────────────────────────────────────
  auto toolbar = CefPanel::CreatePanel(nullptr);
  CefBoxLayoutSettings tb;
  tb.horizontal = true;
  tb.between_child_spacing = 4;
  tb.inside_border_insets = CefInsets(5, 8, 5, 8);
  auto tb_layout = toolbar->SetToBoxLayout(tb);

  toolbar->AddChildView(MakeBtn(this, "←", kBack));
  toolbar->AddChildView(MakeBtn(this, "→", kForward));
  toolbar->AddChildView(MakeBtn(this, "↻", kReload));
  toolbar->AddChildView(MakeBtn(this, "✕", kStop));
  toolbar->AddChildView(MakeBtn(this, "⌂", kHome));

  address_field_ = CefTextfield::CreateTextfield(this);
  address_field_->SetID(kAddress);
  address_field_->SetAccessibleName("Address and search bar");
  toolbar->AddChildView(address_field_);
  tb_layout->SetFlexForView(address_field_, 1);

  bookmark_button_  = MakeBtn(this, "☆", kBookmark);
  download_button_  = MakeBtn(this, "↓", kDownloads);
  update_button_    = MakeBtn(this, "🆕 Update", 12);  // kUpdateAvail=12
  update_button_->SetVisible(false);  // hidden until update found
  shield_button_    = MakeBtn(this, "Shields: On", kShield);

  toolbar->AddChildView(bookmark_button_);
  toolbar->AddChildView(download_button_);
  toolbar->AddChildView(shield_button_);
  toolbar->AddChildView(update_button_);
  window_->AddChildView(toolbar);

  // ── Content panel (overlay — shows only active browser view) ─────────────
  content_panel_ = CefPanel::CreatePanel(nullptr);
  CefBoxLayoutSettings cp;
  cp.horizontal = false;
  content_panel_->SetToBoxLayout(cp);
  window_->AddChildView(content_panel_);
  root_layout->SetFlexForView(content_panel_, 1);

  // ── Download handler ──────────────────────────────────────────────────────
  download_handler_ = new DownloadHandler(
      db_.get(),
      [this](int active) { UpdateDownloadButton(active); });

  // ── Shields persistence ───────────────────────────────────────────────────
  // Will be attached per-tab when tabs are created (each tab has own shields).
  // Global shields for resource filtering use the active tab's shields.

  window_->CenterWindow(CefSize(1280, 800));
  window_->Show();

  // Open first tab.
  const std::string url = startup_url_.empty() ? DefaultHomeUrl() : startup_url_;
  NewTab(url);

  // Start background update check.
  update_checker_ = std::make_unique<UpdateChecker>(
      "0.3.0",
      [this](std::string new_ver, std::string rel_url) {
        OnUpdateAvailable(std::move(new_ver), std::move(rel_url));
      });
  update_checker_->CheckAsync();
}

void BrowserWindow::OnWindowDestroyed(CefRefPtr<CefWindow> /*window*/) {
  CEF_REQUIRE_UI_THREAD();
  address_field_   = nullptr;
  shield_button_   = nullptr;
  bookmark_button_ = nullptr;
  download_button_ = nullptr;
  content_panel_   = nullptr;
  tab_strip_       = nullptr;
  window_          = nullptr;
  tabs_.clear();
}

bool BrowserWindow::CanClose(CefRefPtr<CefWindow> /*window*/) {
  CEF_REQUIRE_UI_THREAD();
  // Close all tab browsers.
  bool all_closed = true;
  for (auto& t : tabs_) {
    if (t.browser) {
      all_closed = false;
      t.browser->GetHost()->TryCloseBrowser();
    }
  }
  return all_closed || tabs_.empty();
}

CefSize BrowserWindow::GetPreferredSize(CefRefPtr<CefView> v) {
  if (v->GetID() == kWindow) return CefSize(1280, 800);
  return CefSize();
}
CefSize BrowserWindow::GetMinimumSize(CefRefPtr<CefView> v) {
  if (v->GetID() == kWindow) return CefSize(800, 500);
  return CefSize();
}
cef_runtime_style_t BrowserWindow::GetWindowRuntimeStyle()  { return CEF_RUNTIME_STYLE_ALLOY; }
cef_runtime_style_t BrowserWindow::GetBrowserRuntimeStyle() { return CEF_RUNTIME_STYLE_ALLOY; }

// ── Tab management ────────────────────────────────────────────────────────────

int BrowserWindow::NewTab(const std::string& url) {
  CEF_REQUIRE_UI_THREAD();

  const int tab_id = next_tab_id_++;
  const int idx    = static_cast<int>(tabs_.size());

  tabs_.emplace_back();
  TabEntry& tab = tabs_.back();
  tab.tab_id = tab_id;
  tab.url    = url.empty() ? DefaultHomeUrl() : url;
  tab.title  = "New Tab";

  // Per-tab shields — attach DB for persistence.
  tab.shields.AttachDatabase(db_.get());
  tab.shields.SetActiveUrl(tab.url);

  // Create browser view.
  CefBrowserSettings bs;
  bs.javascript = STATE_ENABLED;
  tab.browser_view = CefBrowserView::CreateBrowserView(
      this, tab.url, bs, nullptr, nullptr, this);
  tab.browser_view->SetID(kBrowserViewBase + tab_id);
  tab.browser_view->SetVisible(false);  // hide until activated

  // Add to content panel with flex so it fills the panel when visible.
  content_panel_->AddChildView(tab.browser_view);
  if (auto layout = content_panel_->GetLayout()) {
    // Box layout: flex=1 for each child so visible one fills panel.
    static_cast<CefBoxLayout*>(layout.get())->SetFlexForView(tab.browser_view, 1);
  }

  RebuildTabStrip();
  ActivateTab(idx);
  return idx;
}

void BrowserWindow::CloseTab(int tab_index) {
  CEF_REQUIRE_UI_THREAD();
  if (tab_index < 0 || tab_index >= static_cast<int>(tabs_.size())) return;

  TabEntry& tab = tabs_[tab_index];

  // Remove browser view from content panel.
  if (content_panel_ && tab.browser_view)
    content_panel_->RemoveChildView(tab.browser_view);

  // Close browser (fires OnBeforeClose on the browser thread).
  if (tab.browser)
    tab.browser->GetHost()->TryCloseBrowser();

  tabs_.erase(tabs_.begin() + tab_index);

  if (tabs_.empty()) {
    // No tabs left — open a new one or close window.
    if (window_) window_->Close();
    return;
  }

  // Adjust active index.
  int new_active = active_tab_index_;
  if (new_active >= static_cast<int>(tabs_.size()))
    new_active = static_cast<int>(tabs_.size()) - 1;
  active_tab_index_ = -1;  // force re-activate
  ActivateTab(new_active);
}

void BrowserWindow::ActivateTab(int tab_index) {
  CEF_REQUIRE_UI_THREAD();
  if (tab_index < 0 || tab_index >= static_cast<int>(tabs_.size())) return;
  if (tab_index == active_tab_index_) return;

  // Hide current.
  if (active_tab_index_ >= 0 &&
      active_tab_index_ < static_cast<int>(tabs_.size())) {
    tabs_[active_tab_index_].browser_view->SetVisible(false);
  }
  active_tab_index_ = tab_index;

  // Show new.
  TabEntry& tab = tabs_[tab_index];
  tab.browser_view->SetVisible(true);

  if (window_) window_->Layout();

  // Sync toolbar with active tab.
  UpdateToolbar();
  UpdateShieldLabel();
  UpdateBookmarkButton();
  RebuildTabStrip();

  // Focus the browser.
  if (tab.browser) tab.browser->GetHost()->SetFocus(true);
}

int BrowserWindow::FindTabByBrowserId(int browser_id) const {
  for (int i = 0; i < static_cast<int>(tabs_.size()); ++i)
    if (tabs_[i].browser && tabs_[i].browser->GetIdentifier() == browser_id)
      return i;
  return -1;
}

TabEntry* BrowserWindow::ActiveTab() {
  if (active_tab_index_ < 0 ||
      active_tab_index_ >= static_cast<int>(tabs_.size()))
    return nullptr;
  return &tabs_[active_tab_index_];
}
const TabEntry* BrowserWindow::ActiveTab() const {
  if (active_tab_index_ < 0 ||
      active_tab_index_ >= static_cast<int>(tabs_.size()))
    return nullptr;
  return &tabs_[active_tab_index_];
}

// ── Tab strip UI ──────────────────────────────────────────────────────────────

void BrowserWindow::RebuildTabStrip() {
  CEF_REQUIRE_UI_THREAD();
  if (!tab_strip_) return;

  tab_strip_->RemoveAllChildViews();

  for (int i = 0; i < static_cast<int>(tabs_.size()); ++i) {
    const auto& t = tabs_[i];

    // Container panel for each tab: [title_btn] [close_btn]
    auto tab_panel = CefPanel::CreatePanel(nullptr);
    CefBoxLayoutSettings tp;
    tp.horizontal = true;
    tp.between_child_spacing = 0;
    tp.inside_border_insets = CefInsets(0, 0, 0, 0);
    tab_panel->SetToBoxLayout(tp);

    // Title button — shows truncated title.
    std::string label = t.title;
    if (label.size() > 20) label = label.substr(0, 18) + "…";
    if (i == active_tab_index_) label = "▶ " + label;

    auto title_btn = MakeBtn(this, label.c_str(), kTabButtonBase + i);
    title_btn->SetMinimumSize(CefSize(100, 28));
    title_btn->SetMaximumSize(CefSize(200, 32));

    auto close_btn = MakeBtn(this, "×", kTabCloseBase + i);
    close_btn->SetMinimumSize(CefSize(24, 28));
    close_btn->SetMaximumSize(CefSize(24, 32));

    tab_panel->AddChildView(title_btn);
    tab_panel->AddChildView(close_btn);
    tab_strip_->AddChildView(tab_panel);
  }

  // New-tab "+" button.
  auto new_tab_btn = MakeBtn(this, " + ", kNewTab);
  new_tab_btn->SetMinimumSize(CefSize(32, 28));
  tab_strip_->AddChildView(new_tab_btn);

  if (window_) window_->Layout();
}

// ── Toolbar helpers ───────────────────────────────────────────────────────────

void BrowserWindow::UpdateToolbar() {
  const TabEntry* tab = ActiveTab();
  if (!tab) return;

  SetEnabled(window_, kBack,    tab->can_go_back);
  SetEnabled(window_, kForward, tab->can_go_forward);
  SetEnabled(window_, kReload,  !tab->is_loading);
  SetEnabled(window_, kStop,    tab->is_loading);
  SetEnabled(window_, kHome,    true);
  SetEnabled(window_, kShield,  tab->shields.ActiveHost().has_value());

  if (address_field_) {
    if (IsHomeUrl(tab->url))
      address_field_->SetText("");
    else
      address_field_->SetText(tab->url);
  }
}

void BrowserWindow::UpdateShieldLabel() {
  if (!shield_button_) return;
  const TabEntry* tab = ActiveTab();
  if (!tab) { shield_button_->SetText("Shields: N/A"); return; }

  const auto host = tab->shields.ActiveHost();
  shield_button_->SetEnabled(host.has_value());
  if (!host) { shield_button_->SetText("Shields: N/A"); return; }

  const auto snap = tab->stats.Snapshot();
  shield_button_->SetText(
      tab->shields.EnabledForActive()
        ? "Shields: On (" + std::to_string(snap.blocked) + ")"
        : "Shields: Off");
}

void BrowserWindow::UpdateBookmarkButton() {
  if (!bookmark_button_) return;
  const TabEntry* tab = ActiveTab();
  if (!tab || IsInternalUrl(tab->url)) {
    bookmark_button_->SetText("☆");
    bookmark_button_->SetEnabled(false);
    return;
  }
  bookmark_button_->SetEnabled(true);
  const bool bm = db_ && db_->IsOpen() && db_->IsBookmarked(tab->url);
  bookmark_button_->SetText(bm ? "★" : "☆");
}

void BrowserWindow::UpdateDownloadButton(int active_count) {
  CEF_REQUIRE_UI_THREAD();
  if (!download_button_) return;
  if (active_count > 0)
    download_button_->SetText("↓(" + std::to_string(active_count) + ")");
  else
    download_button_->SetText("↓");
}

// ── Script injection ──────────────────────────────────────────────────────────

void BrowserWindow::InjectShieldsScript(CefRefPtr<CefFrame> frame,
                                         TabEntry* tab) {
  if (shields_script_.empty() || !frame || !frame->IsMain()) return;
  const std::string url = frame->GetURL().ToString();
  if (url.rfind("http://", 0) != 0 && url.rfind("https://", 0) != 0) return;
  if (!tab || !tab->shields.EnabledForActive()) return;
  frame->ExecuteJavaScript(shields_script_, url, 0);
}

void BrowserWindow::InjectPageData(CefRefPtr<CefFrame> frame,
                                    const std::string& url) {
  if (!frame || !frame->IsMain()) return;
  if (!db_ || !db_->IsOpen()) return;

  // History page.
  if (url.find("history.html") != std::string::npos) {
    const auto history = db_->GetHistory(500);
    const auto bookmarks = db_->GetBookmarks();

    std::string hist_json = "[";
    for (size_t i = 0; i < history.size(); ++i) {
      const auto& h = history[i];
      if (i) hist_json += ',';
      hist_json += "{\"url\":" + JE(h.url) +
                   ",\"title\":" + JE(h.title) +
                   ",\"visits\":" + std::to_string(h.visit_count) +
                   ",\"last\":" + JE(h.last_visit) + "}";
    }
    hist_json += "]";

    std::string bm_json = "[";
    for (size_t i = 0; i < bookmarks.size(); ++i) {
      const auto& b = bookmarks[i];
      if (i) bm_json += ',';
      bm_json += "{\"id\":" + std::to_string(b.id) +
                 ",\"url\":" + JE(b.url) +
                 ",\"title\":" + JE(b.title) +
                 ",\"created\":" + JE(b.created_at) + "}";
    }
    bm_json += "]";

    frame->ExecuteJavaScript(
        "window.__kingfnHistory=" + hist_json + ";"
        "window.__kingfnBookmarks=" + bm_json + ";"
        "if(typeof loadData==='function') loadData(window.__kingfnHistory, window.__kingfnBookmarks);",
        url, 0);
    return;
  }

  // Downloads page.
  if (url.find("downloads.html") != std::string::npos) {
    const auto downloads = db_->GetDownloads(100);
    std::string dl_json = "[";
    for (size_t i = 0; i < downloads.size(); ++i) {
      const auto& d = downloads[i];
      if (i) dl_json += ',';
      const int pct = d.total_bytes > 0
          ? static_cast<int>(d.received_bytes * 100 / d.total_bytes) : 0;
      dl_json += "{\"id\":" + std::to_string(d.id) +
                 ",\"url\":" + JE(d.url) +
                 ",\"filename\":" + JE(d.filename) +
                 ",\"savePath\":" + JE(d.save_path) +
                 ",\"status\":" + JE(d.status) +
                 ",\"pct\":" + std::to_string(pct) +
                 ",\"started\":" + JE(d.started_at) + "}";
    }
    dl_json += "]";
    frame->ExecuteJavaScript(
        "window.__kingfnDownloads=" + dl_json + ";"
        "if(typeof loadDownloads==='function') loadDownloads(window.__kingfnDownloads);",
        url, 0);
  }
}

// ── CefLoadHandler ────────────────────────────────────────────────────────────

void BrowserWindow::OnLoadStart(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                TransitionType /*tt*/) {
  CEF_REQUIRE_UI_THREAD();
  int idx = FindTabByBrowserId(browser->GetIdentifier());
  if (idx < 0) return;
  InjectShieldsScript(frame, &tabs_[idx]);
}

void BrowserWindow::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              int /*status*/) {
  CEF_REQUIRE_UI_THREAD();
  int idx = FindTabByBrowserId(browser->GetIdentifier());
  if (idx < 0) return;
  InjectShieldsScript(frame, &tabs_[idx]);
  InjectPageData(frame, frame->GetURL().ToString());

  // Record in history (skip internal pages).
  const std::string url = frame->GetURL().ToString();
  if (frame->IsMain() && !IsInternalUrl(url) &&
      url.rfind("http", 0) == 0 && db_ && db_->IsOpen()) {
    db_->AddHistory(url, tabs_[idx].title);
  }
}

void BrowserWindow::OnLoadError(CefRefPtr<CefBrowser> /*browser*/,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode error_code,
                                const CefString& error_text,
                                const CefString& failed_url) {
  CEF_REQUIRE_UI_THREAD();
  if (error_code == ERR_ABORTED || error_code == ERR_BLOCKED_BY_CLIENT) return;
  if (!frame->IsMain()) return;
  const std::string html =
      "<html><head><title>Page unavailable - KINGFN</title>"
      "<style>body{background:#0a0e17;color:#f0f4f8;font-family:system-ui;"
      "display:flex;flex-direction:column;align-items:center;"
      "justify-content:center;min-height:100vh;gap:12px;margin:0}"
      "h1{color:#3df5c4}code{background:#111726;padding:3px 8px;"
      "border-radius:5px;color:#23aaff}</style></head><body>"
      "<h1>&#128737; Can't reach this page</h1>"
      "<p>Couldn't connect to <code>" + failed_url.ToString() + "</code></p>"
      "<p style='color:#8899ac'>" + error_text.ToString() + "</p>"
      "</body></html>";
  frame->LoadURL("data:text/html;charset=utf-8," +
                 CefURIEncode(html, false).ToString());
}

void BrowserWindow::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                          bool is_loading,
                                          bool can_go_back,
                                          bool can_go_forward) {
  CEF_REQUIRE_UI_THREAD();
  int idx = FindTabByBrowserId(browser->GetIdentifier());
  if (idx < 0) return;
  tabs_[idx].is_loading     = is_loading;
  tabs_[idx].can_go_back    = can_go_back;
  tabs_[idx].can_go_forward = can_go_forward;
  if (idx == active_tab_index_) UpdateToolbar();
}

// ── CefDisplayHandler ─────────────────────────────────────────────────────────

void BrowserWindow::OnTitleChange(CefRefPtr<CefBrowser> browser,
                                  const CefString& title) {
  CEF_REQUIRE_UI_THREAD();
  int idx = FindTabByBrowserId(browser->GetIdentifier());
  if (idx < 0) return;
  const std::string t = title.ToString();
  tabs_[idx].title = t.empty() ? "New Tab" : t;
  if (window_ && idx == active_tab_index_)
    window_->SetTitle((t.empty() ? std::string("KINGFN") : t + " - KINGFN"));
  RebuildTabStrip();
}

void BrowserWindow::OnAddressChange(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    const CefString& url) {
  CEF_REQUIRE_UI_THREAD();
  if (!frame->IsMain()) return;
  int idx = FindTabByBrowserId(browser->GetIdentifier());
  if (idx < 0) return;
  const std::string url_str = url.ToString();
  tabs_[idx].url = url_str;
  tabs_[idx].shields.SetActiveUrl(url_str);
  if (idx == active_tab_index_) {
    if (address_field_)
      address_field_->SetText(IsHomeUrl(url_str) ? CefString("") : url);
    UpdateShieldLabel();
    UpdateBookmarkButton();
  }
  // Re-inject shields on SPA navigation.
  InjectShieldsScript(frame, &tabs_[idx]);
}

void BrowserWindow::OnFullscreenModeChange(CefRefPtr<CefBrowser> /*b*/,
                                           bool fullscreen) {
  CEF_REQUIRE_UI_THREAD();
  is_fullscreen_ = fullscreen;
  if (!window_) return;
  if (fullscreen) {
    if (tab_strip_) tab_strip_->SetVisible(false);
    auto tb = window_->GetViewForID(kAddress);
    if (tb) { auto p = tb->GetParentView(); if (p) p->SetVisible(false); }
  } else {
    if (tab_strip_) tab_strip_->SetVisible(true);
    auto tb = window_->GetViewForID(kAddress);
    if (tb) { auto p = tb->GetParentView(); if (p) p->SetVisible(true); }
  }
  window_->SetFullscreen(fullscreen);
  window_->Layout();
}

bool BrowserWindow::OnConsoleMessage(CefRefPtr<CefBrowser>, cef_log_severity_t,
                                      const CefString&, const CefString&,
                                      int) {
  return false;
}

// ── CefLifeSpanHandler ────────────────────────────────────────────────────────

bool BrowserWindow::OnBeforePopup(CefRefPtr<CefBrowser> /*browser*/,
                                  CefRefPtr<CefFrame> /*frame*/,
                                  const CefString& target_url,
                                  const CefString& /*target_frame_name*/,
                                  WindowOpenDisposition disposition,
                                  bool /*user_gesture*/,
                                  const CefPopupFeatures& /*features*/,
                                  CefWindowInfo& /*window_info*/,
                                  CefRefPtr<CefClient>& /*client*/,
                                  CefBrowserSettings& /*settings*/,
                                  CefRefPtr<CefDictionaryValue>& /*extra*/,
                                  bool* /*no_js*/) {
  CEF_REQUIRE_UI_THREAD();
  if (target_url.empty()) return true;
  const std::string url = target_url.ToString();
  if (disposition == WOD_NEW_BACKGROUND_TAB ||
      disposition == WOD_NEW_FOREGROUND_TAB ||
      disposition == WOD_NEW_POPUP ||
      disposition == WOD_NEW_WINDOW) {
    // Open in new KINGFN tab.
    const int idx = NewTab(url);
    if (disposition == WOD_NEW_FOREGROUND_TAB ||
        disposition == WOD_NEW_POPUP ||
        disposition == WOD_NEW_WINDOW) {
      ActivateTab(idx);
    }
  } else {
    // Navigate current tab.
    if (auto* tab = ActiveTab(); tab && tab->browser)
      tab->browser->GetMainFrame()->LoadURL(url);
  }
  return true;  // Cancel default popup.
}

void BrowserWindow::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  // Associate the new browser with the tab that owns this browser view.
  for (auto& t : tabs_) {
    if (t.browser) continue;
    if (t.browser_view && t.browser_view->GetBrowser() &&
        t.browser_view->GetBrowser()->GetIdentifier() ==
            browser->GetIdentifier()) {
      t.browser = browser;
      return;
    }
  }
  // Fallback: assign to last tab that has no browser yet.
  for (auto& t : tabs_) {
    if (!t.browser) { t.browser = browser; return; }
  }
}

void BrowserWindow::OnBeforeClose(CefRefPtr<CefBrowser> /*browser*/) {
  CEF_REQUIRE_UI_THREAD();
  // Nothing needed — browser was already removed from tabs_ in CloseTab().
  if (tabs_.empty() && window_) CefQuitMessageLoop();
}

// ── Keyboard ──────────────────────────────────────────────────────────────────

bool BrowserWindow::OnPreKeyEvent(CefRefPtr<CefBrowser> browser,
                                  const CefKeyEvent& ev,
                                  CefEventHandle /*os*/,
                                  bool* /*shortcut*/) {
  CEF_REQUIRE_UI_THREAD();
  if (ev.type != KEYEVENT_RAWKEYDOWN) return false;

  const bool ctrl  = (ev.modifiers & EVENTFLAG_CONTROL_DOWN) != 0;
  const bool alt   = (ev.modifiers & EVENTFLAG_ALT_DOWN)     != 0;
  const bool shift = (ev.modifiers & EVENTFLAG_SHIFT_DOWN)   != 0;

  TabEntry* tab = ActiveTab();

  // Fullscreen exit.
  if (is_fullscreen_ && (ev.windows_key_code == VK_ESCAPE ||
                          ev.windows_key_code == VK_F11)) {
    OnFullscreenModeChange(browser, false); return true;
  }

  switch (ev.windows_key_code) {
    case 'T': if (ctrl) { NewTab(DefaultHomeUrl()); return true; } break;
    case 'W': if (ctrl) { CloseTab(active_tab_index_); return true; } break;
    case 'L': if (ctrl) { FocusAddressBar(); return true; } break;
    case 'D': if (ctrl) { ToggleBookmark(); return true; } break;
    case 'H': if (ctrl) {
      Navigate("file:///" +
               std::filesystem::path(ExecutableDirectory())
                   .append("resources/history.html")
                   .lexically_normal().string());
      return true;
    } break;
    case 'J': if (ctrl) {  // Ctrl+J = downloads
      Navigate("file:///" +
               std::filesystem::path(ExecutableDirectory())
                   .append("resources/downloads.html")
                   .lexically_normal().string());
      return true;
    } break;
    case 'R':
      if (ctrl && shift) { if (tab && tab->browser) tab->browser->ReloadIgnoreCache(); return true; }
      if (ctrl)          { if (tab && tab->browser) tab->browser->Reload(); return true; }
      break;
    case VK_F5:
      if (shift) { if (tab && tab->browser) tab->browser->ReloadIgnoreCache(); return true; }
      if (tab && tab->browser) tab->browser->Reload(); return true;
    case VK_F11: OnFullscreenModeChange(browser, !is_fullscreen_); return true;
    case VK_TAB:
      if (ctrl) {
        const int n = static_cast<int>(tabs_.size());
        if (n > 1) {
          int next = (active_tab_index_ + (shift ? -1 : 1) + n) % n;
          ActivateTab(next);
        }
        return true;
      }
      break;
    case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
      if (ctrl) {
        int i = ev.windows_key_code - '1';
        if (i < static_cast<int>(tabs_.size())) ActivateTab(i);
        return true;
      }
      break;
    case VK_LEFT:  if (alt && tab && tab->browser && tab->can_go_back)    { tab->browser->GoBack();    return true; } break;
    case VK_RIGHT: if (alt && tab && tab->browser && tab->can_go_forward) { tab->browser->GoForward(); return true; } break;
    case VK_BACK:  if (alt && tab && tab->browser && tab->can_go_back)    { tab->browser->GoBack();    return true; } break;
    case VK_HOME:  if (alt && tab && tab->browser) { tab->browser->GetMainFrame()->LoadURL(startup_url_.empty() ? DefaultHomeUrl() : startup_url_); return true; } break;
    case VK_ESCAPE:
      if (!is_fullscreen_ && tab && tab->browser) tab->browser->StopLoad();
      return true;
    default: break;
  }
  return false;
}

// ── Button & text field ───────────────────────────────────────────────────────

void BrowserWindow::OnButtonPressed(CefRefPtr<CefButton> button) {
  CEF_REQUIRE_UI_THREAD();
  const int id = button->GetID();
  TabEntry* tab = ActiveTab();

  if (id == kNewTab)  { NewTab(DefaultHomeUrl()); return; }
  if (id == 12) {  // kUpdateAvail
    Navigate("https://github.com/Farhan0629/Bravelike/releases/latest");
    return;
  }
  if (id == kBookmark) { ToggleBookmark(); return; }
  if (id == kDownloads) {
    Navigate("file:///" +
             std::filesystem::path(ExecutableDirectory())
                 .append("resources/downloads.html")
                 .lexically_normal().string());
    return;
  }

  // Tab title buttons — switch to that tab.
  if (id >= kTabButtonBase && id < kTabButtonBase + 200) {
    ActivateTab(id - kTabButtonBase); return;
  }
  // Tab close buttons.
  if (id >= kTabCloseBase && id < kTabCloseBase + 200) {
    CloseTab(id - kTabCloseBase); return;
  }

  if (!tab || !tab->browser) return;
  switch (id) {
    case kBack:    tab->browser->GoBack();    break;
    case kForward: tab->browser->GoForward(); break;
    case kReload:  tab->browser->Reload();    break;
    case kStop:    tab->browser->StopLoad();  break;
    case kHome: {
      const std::string home = startup_url_.empty() ? DefaultHomeUrl() : startup_url_;
      tab->browser->GetMainFrame()->LoadURL(home);
      break;
    }
    case kShield:
      tab->shields.ToggleActive();
      UpdateShieldLabel();
      tab->browser->Reload();
      break;
    default: break;
  }
}

bool BrowserWindow::OnKeyEvent(CefRefPtr<CefTextfield> tf,
                               const CefKeyEvent& ev) {
  CEF_REQUIRE_UI_THREAD();
  if (tf->GetID() != kAddress || ev.type != KEYEVENT_RAWKEYDOWN) return false;
  if (ev.windows_key_code == VK_RETURN) {
    Navigate(tf->GetText().ToString());
    TabEntry* tab = ActiveTab();
    if (tab && tab->browser) tab->browser->GetHost()->SetFocus(true);
    return true;
  }
  if (ev.windows_key_code == VK_ESCAPE) {
    TabEntry* tab = ActiveTab();
    if (tab) {
      tf->SetText(IsHomeUrl(tab->url) ? CefString("") : CefString(tab->url));
      if (tab->browser) tab->browser->GetHost()->SetFocus(true);
    }
    return true;
  }
  return false;
}

// ── Navigation ────────────────────────────────────────────────────────────────

void BrowserWindow::Navigate(const std::string& input) {
  TabEntry* tab = ActiveTab();
  if (!tab || !tab->browser) return;
  const std::string url = ResolveAddressInput(input);
  if (!url.empty()) tab->browser->GetMainFrame()->LoadURL(url);
}

void BrowserWindow::FocusAddressBar() {
  if (!address_field_) return;
  address_field_->RequestFocus();
  address_field_->SelectAll(false);
}

void BrowserWindow::ToggleBookmark() {
  TabEntry* tab = ActiveTab();
  if (!tab || IsInternalUrl(tab->url) || !db_ || !db_->IsOpen()) return;
  const int64_t bm_id = db_->GetBookmarkId(tab->url);
  if (bm_id != -1) {
    db_->RemoveBookmark(bm_id);
  } else {
    db_->AddBookmark(tab->url, tab->title);
  }
  UpdateBookmarkButton();
}

// ── Resource blocking ─────────────────────────────────────────────────────────

CefRefPtr<CefResourceRequestHandler> BrowserWindow::GetResourceRequestHandler(
    CefRefPtr<CefBrowser> /*b*/, CefRefPtr<CefFrame> /*f*/,
    CefRefPtr<CefRequest> /*r*/, bool /*nav*/, bool /*dl*/,
    const CefString& /*initiator*/, bool& /*disable*/) {
  return this;
}

cef_return_value_t BrowserWindow::OnBeforeResourceLoad(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> /*frame*/,
    CefRefPtr<CefRequest> request,
    CefRefPtr<CefCallback> /*callback*/) {
  int idx = FindTabByBrowserId(browser->GetIdentifier());
  if (idx < 0) return RV_CONTINUE;

  TabEntry& tab = tabs_[idx];
  const bool enabled = tab.shields.EnabledForActive();
  const auto decision = enabled
      ? filter_engine_.Evaluate(request->GetURL().ToString())
      : FilterDecision{};
  const bool blocked = enabled && decision.action == FilterAction::kBlock;
  tab.stats.Record(blocked);
  if (idx == active_tab_index_ && blocked) UpdateShieldLabel();
  return blocked ? RV_CANCEL : RV_CONTINUE;
}



// ── Update available notification ────────────────────────────────────────────

void BrowserWindow::OnUpdateAvailable(std::string new_ver, std::string url) {
  CEF_REQUIRE_UI_THREAD();
  if (!update_button_ || !window_) return;
  // Store the release URL so the button can navigate to it.
  // Show the update button in the toolbar.
  update_button_->SetText("🆕 v" + new_ver);
  update_button_->SetVisible(true);
  window_->Layout();
  // Also show a one-time notification via injected JS on the active tab.
  if (TabEntry* tab = ActiveTab(); tab && tab->browser) {
    const std::string js =
        "(function(){"
        "var b=document.createElement('div');"
        "b.style.cssText='position:fixed;top:0;left:0;right:0;z-index:999999;"
        "background:linear-gradient(135deg,#3df5c4,#23aaff);color:#0a0e17;"
        "font-family:system-ui;font-weight:700;padding:10px 20px;"
        "display:flex;align-items:center;justify-content:space-between;';"
        "b.innerHTML='<span>&#128081; KINGFN v" + new_ver + " is available!</span>"
        "<a href="" + url + "" style="color:#0a0e17;text-decoration:underline;"
        "font-size:.9rem">Download &rarr;</a>';"
        "document.body.prepend(b);"
        "setTimeout(function(){b.remove();},12000);"
        "})();";
    tab->browser->GetMainFrame()->ExecuteJavaScript(js, "", 0);
  }
}

}  // namespace kingfn
