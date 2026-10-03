#include "src/browser/update_checker.h"

#include <windows.h>
#include <winhttp.h>

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "include/cef_task.h"
#include "include/wrapper/cef_helpers.h"

// Link WinHTTP — also declared in CMakeLists.
#pragma comment(lib, "winhttp.lib")

namespace kingfn {

// ── Version ───────────────────────────────────────────────────────────────────

Version Version::Parse(const std::string& s) {
  Version v;
  // Strip leading 'v' or 'V'.
  const char* p = s.c_str();
  if (*p == 'v' || *p == 'V') ++p;
  std::sscanf(p, "%d.%d.%d", &v.major, &v.minor, &v.patch);
  return v;
}

bool Version::operator>(const Version& o) const {
  if (major != o.major) return major > o.major;
  if (minor != o.minor) return minor > o.minor;
  return patch > o.patch;
}

std::string Version::ToString() const {
  return std::to_string(major) + "." +
         std::to_string(minor) + "." +
         std::to_string(patch);
}

// ── HTTP helper (WinHTTP) ─────────────────────────────────────────────────────

namespace {

// Very small JSON field extractor — avoids adding a JSON library dependency.
// Finds the first occurrence of `"key":"value"` or `"key": "value"`.
std::string ExtractJsonString(const std::string& json, const std::string& key) {
  const std::string needle = "\"" + key + "\"";
  auto pos = json.find(needle);
  if (pos == std::string::npos) return {};
  pos = json.find('"', pos + needle.size());
  if (pos == std::string::npos) return {};
  // Skip optional ': ' between key and value.
  while (pos < json.size() && (json[pos] == '"' || json[pos] == ':' ||
                                json[pos] == ' ')) ++pos;
  if (pos >= json.size() || json[pos - 1] != '"') return {};
  // Now pos points to first char of value.  Find closing quote.
  std::string result;
  while (pos < json.size() && json[pos] != '"') {
    if (json[pos] == '\\') { ++pos; }
    result += json[pos++];
  }
  return result;
}

struct WinHttpGuard {
  HINTERNET h{nullptr};
  explicit WinHttpGuard(HINTERNET h) : h(h) {}
  ~WinHttpGuard() { if (h) WinHttpCloseHandle(h); }
  operator HINTERNET() const { return h; }
  bool ok() const { return h != nullptr; }
};

std::string WinHttpGet(const std::wstring& host, const std::wstring& path) {
  WinHttpGuard session{WinHttpOpen(
      L"KINGFNBrowser/0.3 (update-check; Windows)",
      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
      WINHTTP_NO_PROXY_NAME,
      WINHTTP_NO_PROXY_BYPASS, 0)};
  if (!session.ok()) return {};

  WinHttpGuard connect{WinHttpConnect(session, host.c_str(),
                                      INTERNET_DEFAULT_HTTPS_PORT, 0)};
  if (!connect.ok()) return {};

  WinHttpGuard request{WinHttpOpenRequest(
      connect, L"GET", path.c_str(), nullptr,
      WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
      WINHTTP_FLAG_SECURE)};
  if (!request.ok()) return {};

  // Set timeout: 5 s connect, 10 s receive.
  DWORD timeout_connect = 5000, timeout_receive = 10000;
  WinHttpSetOption(request, WINHTTP_OPTION_CONNECT_TIMEOUT,
                   &timeout_connect, sizeof(timeout_connect));
  WinHttpSetOption(request, WINHTTP_OPTION_RECEIVE_TIMEOUT,
                   &timeout_receive, sizeof(timeout_receive));

  if (!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                           WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
    return {};
  if (!WinHttpReceiveResponse(request, nullptr)) return {};

  std::string body;
  DWORD avail = 0;
  while (WinHttpQueryDataAvailable(request, &avail) && avail > 0) {
    std::string chunk(avail, '\0');
    DWORD read = 0;
    WinHttpReadData(request, chunk.data(), avail, &read);
    body.append(chunk.data(), read);
  }
  return body;
}

}  // namespace

// ── UpdateChecker ─────────────────────────────────────────────────────────────

UpdateChecker::UpdateChecker(
    std::string current_version,
    std::function<void(std::string, std::string)> on_update_available)
    : current_version_(std::move(current_version)),
      callback_(std::move(on_update_available)) {}

UpdateChecker::~UpdateChecker() {
  if (thread_.joinable()) thread_.detach();
}

void UpdateChecker::CheckAsync() {
  thread_ = std::thread([this] { DoCheck(); });
}

void UpdateChecker::DoCheck() {
  try {
    const std::string json = WinHttpGet(
        L"api.github.com",
        L"/repos/Farhan0629/Bravelike/releases/latest");
    if (json.empty()) return;

    const std::string tag  = ExtractJsonString(json, "tag_name");
    const std::string url  = ExtractJsonString(json, "html_url");
    if (tag.empty()) return;

    const Version latest  = Version::Parse(tag);
    const Version current = Version::Parse(current_version_);

    if (latest > current) {
      // Post result back to the CEF UI thread.
      auto cb = callback_;
      const std::string new_ver = latest.ToString();
      const std::string rel_url = url.empty()
          ? "https://github.com/Farhan0629/Bravelike/releases/latest"
          : url;
      CefPostTask(TID_UI, base::BindOnce(
          [](std::function<void(std::string, std::string)> f,
             std::string v, std::string u) { f(v, u); },
          std::move(cb), new_ver, rel_url));
    }
  } catch (...) {
    // Silently ignore network / parse errors.
  }
}

}  // namespace kingfn
