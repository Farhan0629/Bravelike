#include "core/navigation.h"
#include <cctype>
#include <iomanip>
#include <sstream>

namespace bravelike {
namespace {
std::string Trim(std::string_view value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string_view::npos) return {};
  const auto last = value.find_last_not_of(" \t\r\n");
  return std::string(value.substr(first, last - first + 1));
}

bool HasExplicitScheme(std::string_view input) {
  const auto position = input.find("://");
  if (position == std::string_view::npos || position == 0) return false;
  for (std::size_t i = 0; i < position; ++i) {
    const unsigned char c = static_cast<unsigned char>(input[i]);
    if (!(std::isalnum(c) || c == '+' || c == '-' || c == '.')) return false;
  }
  return true;
}
}  // namespace

std::string UrlEncodeQuery(std::string_view input) {
  std::ostringstream result;
  result << std::uppercase << std::hex;
  for (const unsigned char c : input) {
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      result << static_cast<char>(c);
    } else if (c == ' ') {
      result << '+';
    } else {
      result << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
  }
  return result.str();
}

std::string ResolveAddressInput(std::string_view input) {
  const std::string value = Trim(input);
  if (value.empty()) return {};
  if (HasExplicitScheme(value)) return value;

  const bool looks_like_host = value.find(' ') == std::string::npos &&
      (value.find('.') != std::string::npos || value == "localhost" ||
       value.rfind("localhost:", 0) == 0);
  if (looks_like_host) return "https://" + value;
  return "https://search.brave.com/search?q=" + UrlEncodeQuery(value);
}
}  // namespace bravelike
