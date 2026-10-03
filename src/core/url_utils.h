#pragma once
#include <optional>
#include <string>
#include <string_view>

namespace kingfn {
std::string ToLowerAscii(std::string_view input);
std::optional<std::string> ExtractHttpHost(std::string_view url);
bool IsDomainOrSubdomain(std::string_view host, std::string_view domain);
}  // namespace kingfn
