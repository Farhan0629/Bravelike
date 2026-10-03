#pragma once
#include <string_view>
namespace kingfn {
// Hide only the exact browser-owned home URL, never an external lookalike.
inline bool IsExactHomeUrl(std::string_view url, std::string_view home_url) {
  return !home_url.empty() && url == home_url;
}
inline bool IsHomeAlias(std::string_view input) {
  return input == "home" || input == "kingfn" || input == "kingfn://home" ||
      input == "kingfn://newtab" || input == "about:home";
}
}
