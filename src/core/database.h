#pragma once
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

// Forward-declare sqlite3 so including this header doesn't require sqlite3.h.
struct sqlite3;
struct sqlite3_stmt;

namespace kingfn {

struct HistoryEntry {
  std::string url;
  std::string title;
  int64_t     visit_count{0};
  std::string last_visit;   // ISO-8601 text
};

struct BookmarkEntry {
  int64_t     id{0};
  std::string url;
  std::string title;
  std::string created_at;
};

struct DownloadRecord {
  int64_t     id{0};
  std::string url;
  std::string filename;
  std::string save_path;
  int64_t     total_bytes{0};
  int64_t     received_bytes{0};
  std::string status;      // "downloading" | "complete" | "cancelled" | "error"
  std::string started_at;
  std::string completed_at;
};

class Database {
 public:
  // Opens (or creates) the SQLite database at the given file path.
  explicit Database(const std::string& path);
  ~Database();

  // Not copyable.
  Database(const Database&) = delete;
  Database& operator=(const Database&) = delete;

  bool IsOpen() const { return db_ != nullptr; }

  // ── History ────────────────────────────────────────────────────────────────
  void AddHistory(const std::string& url, const std::string& title);
  std::vector<HistoryEntry> GetHistory(int limit = 200,
                                        const std::string& query = "");
  void ClearHistory();

  // ── Bookmarks ─────────────────────────────────────────────────────────────
  int64_t     AddBookmark(const std::string& url, const std::string& title);
  void        RemoveBookmark(int64_t id);
  bool        IsBookmarked(const std::string& url);
  int64_t     GetBookmarkId(const std::string& url);  // -1 if not found
  std::vector<BookmarkEntry> GetBookmarks();

  // ── Shields preferences ───────────────────────────────────────────────────
  void SetShieldsEnabled(const std::string& host, bool enabled);
  bool GetShieldsEnabled(const std::string& host, bool default_val = true);
  std::unordered_set<std::string> GetAllDisabledHosts();

  // ── Downloads ─────────────────────────────────────────────────────────────
  int64_t AddDownload(const std::string& url, const std::string& filename,
                      const std::string& save_path, int64_t total_bytes);
  void UpdateDownload(int64_t id, int64_t received, int64_t total,
                      const std::string& status);
  void CompleteDownload(int64_t id, const std::string& save_path);
  std::vector<DownloadRecord> GetDownloads(int limit = 100);
  void ClearCompletedDownloads();

 private:
  sqlite3* db_{nullptr};
  void CreateTables();
  void Execute(const char* sql);
  sqlite3_stmt* Prepare(const char* sql);
  static std::string NowIso();
};

}  // namespace kingfn
