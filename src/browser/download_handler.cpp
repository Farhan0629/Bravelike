#include "src/browser/download_handler.h"

#include <filesystem>
#include <windows.h>
#include <shlobj.h>

#include "include/cef_browser.h"
#include "include/wrapper/cef_helpers.h"

namespace kingfn {

DownloadHandler::DownloadHandler(Database* db,
                                 DownloadBadgeCallback on_badge_change)
    : db_(db), on_badge_change_(std::move(on_badge_change)) {}

bool DownloadHandler::CanDownload(CefRefPtr<CefBrowser> /*browser*/,
                                  const CefString& /*url*/,
                                  const CefString& /*request_method*/) {
  return true;
}

std::string DownloadHandler::DownloadsFolder() {
  wchar_t path[MAX_PATH]{};
  if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_MYDOCUMENTS, nullptr,
                                  SHGFP_TYPE_CURRENT, path))) {
    auto base = std::filesystem::path(path).parent_path() / L"Downloads";
    std::filesystem::create_directories(base);
    return base.string();
  }
  return ".";
}

void DownloadHandler::OnBeforeDownload(
    CefRefPtr<CefBrowser> /*browser*/,
    CefRefPtr<CefDownloadItem> item,
    const CefString& suggested_name,
    CefRefPtr<CefBeforeDownloadCallback> callback) {
  CEF_REQUIRE_UI_THREAD();

  const std::string folder = DownloadsFolder();
  const std::string filename = suggested_name.ToString();
  const std::string save_path =
      (std::filesystem::path(folder) / filename).string();

  // Record in DB.
  int64_t db_id = -1;
  if (db_ && db_->IsOpen()) {
    db_id = db_->AddDownload(item->GetURL().ToString(), filename,
                             save_path, item->GetTotalBytes());
  }

  {
    std::lock_guard<std::mutex> g(mutex_);
    cef_id_to_db_id_[item->GetId()] = db_id;
    ++active_count_;
  }

  if (on_badge_change_) on_badge_change_(ActiveDownloadCount());

  // Show OS save-file dialog.
  callback->Continue(save_path, /*show_dialog=*/true);
}

void DownloadHandler::OnDownloadUpdated(
    CefRefPtr<CefBrowser> /*browser*/,
    CefRefPtr<CefDownloadItem> item,
    CefRefPtr<CefDownloadItemCallback> /*callback*/) {
  CEF_REQUIRE_UI_THREAD();

  int64_t db_id = -1;
  {
    std::lock_guard<std::mutex> g(mutex_);
    auto it = cef_id_to_db_id_.find(item->GetId());
    if (it != cef_id_to_db_id_.end()) db_id = it->second;
  }

  std::string status = "downloading";
  if (item->IsCancelled())  status = "cancelled";
  else if (item->IsComplete()) status = "complete";

  if (db_ && db_->IsOpen() && db_id != -1) {
    if (item->IsComplete()) {
      db_->CompleteDownload(db_id, item->GetFullPath().ToString());
    } else {
      db_->UpdateDownload(db_id, item->GetReceivedBytes(),
                          item->GetTotalBytes(), status);
    }
  }

  if (item->IsComplete() || item->IsCancelled()) {
    std::lock_guard<std::mutex> g(mutex_);
    cef_id_to_db_id_.erase(item->GetId());
    if (active_count_ > 0) --active_count_;
    if (on_badge_change_) on_badge_change_(active_count_);
  }
}

int DownloadHandler::ActiveDownloadCount() const {
  std::lock_guard<std::mutex> g(mutex_);
  return active_count_;
}

}  // namespace kingfn
