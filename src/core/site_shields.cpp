#include "core/site_shields.h"

#include "core/url_utils.h"

namespace bravelike {
void SiteShields::SetActiveUrl(std::string_view url) {
  auto host = ExtractHttpHost(url);
  std::lock_guard<std::mutex> guard(mutex_);
  active_host_ = std::move(host);
}

std::optional<std::string> SiteShields::ActiveHost() const {
  std::lock_guard<std::mutex> guard(mutex_);
  return active_host_;
}

bool SiteShields::EnabledForActive() const {
  std::lock_guard<std::mutex> guard(mutex_);
  return active_host_ && disabled_hosts_.count(*active_host_) == 0;
}

bool SiteShields::ToggleActive() {
  std::lock_guard<std::mutex> guard(mutex_);
  if (!active_host_) return false;
  if (disabled_hosts_.count(*active_host_)) {
    disabled_hosts_.erase(*active_host_);
  } else {
    disabled_hosts_.insert(*active_host_);
  }
  return true;
}
}  // namespace bravelike
