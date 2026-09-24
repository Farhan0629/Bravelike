#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>

namespace bravelike {
// Policy is keyed by exact normalized HTTP(S) host. This is deliberately
// independent of the destination of each individual subresource request.
class SiteShields {
 public:
  void SetActiveUrl(std::string_view url);
  std::optional<std::string> ActiveHost() const;
  bool EnabledForActive() const;
  // Returns false when the active page has no HTTP(S) host.
  bool ToggleActive();

 private:
  mutable std::mutex mutex_;
  std::optional<std::string> active_host_;
  std::unordered_set<std::string> disabled_hosts_;
};
}  // namespace bravelike
