#include "core/database.h"
#include <sqlite3.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace kingfn {
namespace {
std::string IsoNow() {
  auto now = std::chrono::system_clock::now();
  auto t   = std::chrono::system_clock::to_time_t(now);
  std::tm tm_buf{};
#ifdef _WIN32
  gmtime_s(&tm_buf, &t);
#else
  gmtime_r(&t, &tm_buf);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}
}  // namespace

Database::Database(const std::string& path) {
  if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
    db_ = nullptr;
    return;
  }
  // Performance pragmas.
  Execute("PRAGMA journal_mode=WAL;");
  Execute("PRAGMA synchronous=NORMAL;");
  Execute("PRAGMA foreign_keys=ON;");
  CreateTables();
}

Database::~Database() {
  if (db_) sqlite3_close(db_);
}

void Database::Execute(const char* sql) {
  char* err = nullptr;
  sqlite3_exec(db_, sql, nullptr, nullptr, &err);
  if (err) sqlite3_free(err);
}

sqlite3_stmt* Database::Prepare(const char* sql) {
  sqlite3_stmt* stmt = nullptr;
  sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
  return stmt;
}

void Database::CreateTables() {
  Execute(R"(
    CREATE TABLE IF NOT EXISTS history (
      id         INTEGER PRIMARY KEY AUTOINCREMENT,
      url        TEXT NOT NULL,
      title      TEXT NOT NULL DEFAULT '',
      visit_count INTEGER NOT NULL DEFAULT 1,
      last_visit  TEXT NOT NULL
    );
    CREATE UNIQUE INDEX IF NOT EXISTS history_url ON history(url);
  )");

  Execute(R"(
    CREATE TABLE IF NOT EXISTS bookmarks (
      id         INTEGER PRIMARY KEY AUTOINCREMENT,
      url        TEXT NOT NULL UNIQUE,
      title      TEXT NOT NULL DEFAULT '',
      created_at TEXT NOT NULL
    );
  )");

  Execute(R"(
    CREATE TABLE IF NOT EXISTS shields_prefs (
      host       TEXT PRIMARY KEY,
      enabled    INTEGER NOT NULL DEFAULT 1
    );
  )");

  Execute(R"(
    CREATE TABLE IF NOT EXISTS downloads (
      id             INTEGER PRIMARY KEY AUTOINCREMENT,
      url            TEXT NOT NULL,
      filename       TEXT NOT NULL,
      save_path      TEXT NOT NULL DEFAULT '',
      total_bytes    INTEGER NOT NULL DEFAULT 0,
      received_bytes INTEGER NOT NULL DEFAULT 0,
      status         TEXT NOT NULL DEFAULT 'downloading',
      started_at     TEXT NOT NULL,
      completed_at   TEXT NOT NULL DEFAULT ''
    );
  )");
}

// ── History ──────────────────────────────────────────────────────────────────

void Database::AddHistory(const std::string& url, const std::string& title) {
  if (!db_ || url.empty()) return;
  const char* sql = R"(
    INSERT INTO history(url, title, visit_count, last_visit)
    VALUES(?, ?, 1, ?)
    ON CONFLICT(url) DO UPDATE SET
      title       = excluded.title,
      visit_count = visit_count + 1,
      last_visit  = excluded.last_visit;
  )";
  auto* stmt = Prepare(sql);
  if (!stmt) return;
  const auto now = IsoNow();
  sqlite3_bind_text(stmt, 1, url.c_str(),   -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, now.c_str(),   -1, SQLITE_TRANSIENT);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
}

std::vector<HistoryEntry> Database::GetHistory(int limit,
                                               const std::string& query) {
  std::vector<HistoryEntry> result;
  if (!db_) return result;

  const char* sql = query.empty()
      ? "SELECT url,title,visit_count,last_visit FROM history ORDER BY last_visit DESC LIMIT ?;"
      : "SELECT url,title,visit_count,last_visit FROM history WHERE url LIKE ? OR title LIKE ? ORDER BY last_visit DESC LIMIT ?;";

  auto* stmt = Prepare(sql);
  if (!stmt) return result;

  if (query.empty()) {
    sqlite3_bind_int(stmt, 1, limit);
  } else {
    const std::string pattern = "%" + query + "%";
    sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, limit);
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    HistoryEntry e;
    auto col = [&](int i) {
      auto* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
      return t ? std::string(t) : std::string{};
    };
    e.url         = col(0);
    e.title       = col(1);
    e.visit_count = sqlite3_column_int64(stmt, 2);
    e.last_visit  = col(3);
    result.push_back(std::move(e));
  }
  sqlite3_finalize(stmt);
  return result;
}

void Database::ClearHistory() {
  if (db_) Execute("DELETE FROM history;");
}

// ── Bookmarks ────────────────────────────────────────────────────────────────

int64_t Database::AddBookmark(const std::string& url, const std::string& title) {
  if (!db_ || url.empty()) return -1;
  const char* sql =
      "INSERT OR IGNORE INTO bookmarks(url,title,created_at) VALUES(?,?,?);";
  auto* stmt = Prepare(sql);
  if (!stmt) return -1;
  sqlite3_bind_text(stmt, 1, url.c_str(),   -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);
  const auto now = IsoNow();
  sqlite3_bind_text(stmt, 3, now.c_str(),   -1, SQLITE_TRANSIENT);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return sqlite3_last_insert_rowid(db_);
}

void Database::RemoveBookmark(int64_t id) {
  if (!db_) return;
  auto* stmt = Prepare("DELETE FROM bookmarks WHERE id=?;");
  if (!stmt) return;
  sqlite3_bind_int64(stmt, 1, id);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
}

bool Database::IsBookmarked(const std::string& url) {
  return GetBookmarkId(url) != -1;
}

int64_t Database::GetBookmarkId(const std::string& url) {
  if (!db_) return -1;
  auto* stmt = Prepare("SELECT id FROM bookmarks WHERE url=? LIMIT 1;");
  if (!stmt) return -1;
  sqlite3_bind_text(stmt, 1, url.c_str(), -1, SQLITE_TRANSIENT);
  int64_t id = -1;
  if (sqlite3_step(stmt) == SQLITE_ROW) id = sqlite3_column_int64(stmt, 0);
  sqlite3_finalize(stmt);
  return id;
}

std::vector<BookmarkEntry> Database::GetBookmarks() {
  std::vector<BookmarkEntry> result;
  if (!db_) return result;
  auto* stmt = Prepare(
      "SELECT id,url,title,created_at FROM bookmarks ORDER BY created_at DESC;");
  if (!stmt) return result;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    BookmarkEntry e;
    e.id         = sqlite3_column_int64(stmt, 0);
    auto col = [&](int i) {
      auto* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
      return t ? std::string(t) : std::string{};
    };
    e.url        = col(1);
    e.title      = col(2);
    e.created_at = col(3);
    result.push_back(std::move(e));
  }
  sqlite3_finalize(stmt);
  return result;
}

// ── Shields preferences ──────────────────────────────────────────────────────

void Database::SetShieldsEnabled(const std::string& host, bool enabled) {
  if (!db_) return;
  const char* sql =
      "INSERT OR REPLACE INTO shields_prefs(host,enabled) VALUES(?,?);";
  auto* stmt = Prepare(sql);
  if (!stmt) return;
  sqlite3_bind_text(stmt, 1, host.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, enabled ? 1 : 0);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
}

bool Database::GetShieldsEnabled(const std::string& host, bool default_val) {
  if (!db_) return default_val;
  auto* stmt = Prepare("SELECT enabled FROM shields_prefs WHERE host=? LIMIT 1;");
  if (!stmt) return default_val;
  sqlite3_bind_text(stmt, 1, host.c_str(), -1, SQLITE_TRANSIENT);
  bool result = default_val;
  if (sqlite3_step(stmt) == SQLITE_ROW)
    result = sqlite3_column_int(stmt, 0) != 0;
  sqlite3_finalize(stmt);
  return result;
}

std::unordered_set<std::string> Database::GetAllDisabledHosts() {
  std::unordered_set<std::string> result;
  if (!db_) return result;
  auto* stmt = Prepare("SELECT host FROM shields_prefs WHERE enabled=0;");
  if (!stmt) return result;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    auto* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    if (t) result.insert(t);
  }
  sqlite3_finalize(stmt);
  return result;
}

// ── Downloads ────────────────────────────────────────────────────────────────

int64_t Database::AddDownload(const std::string& url,
                               const std::string& filename,
                               const std::string& save_path,
                               int64_t total_bytes) {
  if (!db_) return -1;
  const char* sql =
      "INSERT INTO downloads(url,filename,save_path,total_bytes,started_at)"
      " VALUES(?,?,?,?,?);";
  auto* stmt = Prepare(sql);
  if (!stmt) return -1;
  const auto now = IsoNow();
  sqlite3_bind_text(stmt, 1, url.c_str(),       -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, filename.c_str(),   -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, save_path.c_str(),  -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 4, total_bytes);
  sqlite3_bind_text(stmt, 5, now.c_str(),        -1, SQLITE_TRANSIENT);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return sqlite3_last_insert_rowid(db_);
}

void Database::UpdateDownload(int64_t id, int64_t received, int64_t total,
                               const std::string& status) {
  if (!db_) return;
  auto* stmt = Prepare(
      "UPDATE downloads SET received_bytes=?,total_bytes=?,status=? WHERE id=?;");
  if (!stmt) return;
  sqlite3_bind_int64(stmt, 1, received);
  sqlite3_bind_int64(stmt, 2, total);
  sqlite3_bind_text(stmt, 3, status.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 4, id);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
}

void Database::CompleteDownload(int64_t id, const std::string& save_path) {
  if (!db_) return;
  const auto now = IsoNow();
  auto* stmt = Prepare(
      "UPDATE downloads SET status='complete',save_path=?,completed_at=? WHERE id=?;");
  if (!stmt) return;
  sqlite3_bind_text(stmt, 1, save_path.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, now.c_str(),        -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 3, id);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
}

std::vector<DownloadRecord> Database::GetDownloads(int limit) {
  std::vector<DownloadRecord> result;
  if (!db_) return result;
  auto* stmt = Prepare(
      "SELECT id,url,filename,save_path,total_bytes,received_bytes,"
      "status,started_at,completed_at FROM downloads ORDER BY started_at DESC LIMIT ?;");
  if (!stmt) return result;
  sqlite3_bind_int(stmt, 1, limit);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    DownloadRecord r;
    r.id             = sqlite3_column_int64(stmt, 0);
    auto col = [&](int i) {
      auto* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
      return t ? std::string(t) : std::string{};
    };
    r.url            = col(1);
    r.filename       = col(2);
    r.save_path      = col(3);
    r.total_bytes    = sqlite3_column_int64(stmt, 4);
    r.received_bytes = sqlite3_column_int64(stmt, 5);
    r.status         = col(6);
    r.started_at     = col(7);
    r.completed_at   = col(8);
    result.push_back(std::move(r));
  }
  sqlite3_finalize(stmt);
  return result;
}

void Database::ClearCompletedDownloads() {
  if (db_)
    Execute("DELETE FROM downloads WHERE status IN ('complete','cancelled','error');");
}

}  // namespace kingfn
