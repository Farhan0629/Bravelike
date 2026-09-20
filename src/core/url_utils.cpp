#include "core/url_utils.h"
#include <algorithm>
#include <cctype>

namespace bravelike {
std::string ToLowerAscii(std::string_view input) {
  std::string result(input);
  std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return result;
}

std::optional<std::string> ExtractHttpHost(std::string_view url) {
  const auto scheme_end = url.find("://");
  if (scheme_end == std::string_view::npos) return std::nullopt;
  const auto scheme = ToLowerAscii(url.substr(0, scheme_end));
  if (scheme != "http" && scheme != "https") return std::nullopt;

  auto authority = url.substr(scheme_end + 3);
  const auto authority_end = authority.find_first_of("/?#");
  authority = authority.substr(0, authority_end);
  if (authority.empty()) return std::nullopt;

  const auto at = authority.rfind('@');
  if (at != std::string_view::npos) authority.remove_prefix(at + 1);
  if (authority.empty()) return std::nullopt;

  std::string_view host;
  if (authority.front() == '[') {
    const auto close = authority.find(']');
    if (close == std::string_view::npos) return std::nullopt;
    host = authority.substr(1, close - 1);
  } else {
    const auto colon = authority.find(':');
    host = authority.substr(0, colon);
  }
  if (host.empty()) return std::nullopt;
  auto normalized = ToLowerAscii(host);
  while (!normalized.empty() && normalized.back() == '.') normalized.pop_back();
  if (normalized.empty()) return std::nullopt;
  return normalized;
}

bool IsDomainOrSubdomain(std::string_view host, std::string_view domain) {
  if (host == domain) return true;
  return host.size() > domain.size() &&
         host.compare(host.size() - domain.size(), domain.size(), domain) == 0 &&
         host[host.size() - domain.size() - 1] == '.';
}
}  // namespace bravelike
