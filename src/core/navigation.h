#pragma once
#include <string>
#include <string_view>

namespace bravelike {
std::string UrlEncodeQuery(std::string_view input);
std::string ResolveAddressInput(std::string_view input);
}  // namespace bravelike
