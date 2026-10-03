#pragma once
#include <string>
#include <string_view>

namespace kingfn {
std::string UrlEncodeQuery(std::string_view input);
std::string ResolveAddressInput(std::string_view input);
}  // namespace kingfn
