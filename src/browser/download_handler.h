#pragma once
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

#include "include/cef_download_handler.h"
#include "src/core/database.h"

namespace kingfn {

// Called on the UI thread when the download badge count changes.
using DownloadBadgeCallback = std::function<void(int active_count)>;

class DownloadHandler final : public CefDownloadHandler {
 public:
  explicit DownloadHandler(Database* db, DownloadBadgeCallback on_badge_change);

  // CefDownloadHandler overrides.
  bool CanDownload(CefRefPtr<CefBrowser> browser,
                   const CefString& url,
                   const CefString& request_method) override;
  void OnBeforeDownload(CefRefPtr<CefBrowser> browser,
                        CefRefPtr<CefDownloadItem> item,
                        const CefString& suggested_name,
                        CefRefPtr<CefBeforeDownloadCallback> callback) override;
  void OnDownloadUpdated(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefDownloadItem> item,
                         CefRefPtr<CefDownloadItemCallback> callback) override;

  int ActiveDownloadCount() const;

 private:
  Database* db_;  // not owned
  DownloadBadgeCallback on_badge_change_;

  mutable std::mutex mutex_;
  // Maps CEF download ID → our DB row ID.
  std::unordered_map<uint32_t, int64_t> cef_id_to_db_id_;
  int active_count_{0};

  static std::string DownloadsFolder();

  IMPLEMENT_REFCOUNTING(DownloadHandler);
  DISALLOW_COPY_AND_ASSIGN(DownloadHandler);
};

}  // namespace kingfn
