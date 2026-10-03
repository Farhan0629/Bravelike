#pragma once
#include <string>
#include <string_view>
namespace kingfn {
inline bool IsExactHomeUrl(std::string_view url, std::string_view home_url) {
  return !home_url.empty() && url == home_url;
}
inline bool IsHomeAlias(std::string_view input) {
  return input == "home" || input == "kingfn" || input == "kingfn://home" || input == "kingfn://newtab" || input == "about:home";
}
// Input is an absolute Windows path encoded as UTF-8. Preserve path separators
// and the drive colon; escape reserved URL characters and non-ASCII bytes.
inline std::string WindowsPathToFileUrl(std::string_view path) {
  std::string normalized(path);
  if (normalized.rfind("\\\\?\\", 0) == 0) normalized.erase(0, 4);
  for (char& c : normalized) if (c == '\\') c = '/';
  std::string output = normalized.rfind("//", 0) == 0 ? "file:" : "file:///";
  const char* hex = "0123456789ABCDEF";
  for (unsigned char c : normalized) {
    const bool plain = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~' || c == '/' || c == ':';
    if (plain) output.push_back(static_cast<char>(c));
    else { output.push_back('%'); output.push_back(hex[c >> 4]); output.push_back(hex[c & 15]); }
  }
  return output;
}
}
