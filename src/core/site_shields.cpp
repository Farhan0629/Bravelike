#include "core/site_shields.h"
#include "core/database.h"
#include "core/url_utils.h"

namespace kingfn {

void SiteShields::AttachDatabase(Database* db) {
  std::lock_guard<std::mutex> guard(mutex_);
  db_ = db;
  if (db_ && db_->IsOpen()) {
    disabled_hosts_ = db_->GetAllDisabledHosts();
  }
}

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
  const bool currently_enabled = disabled_hosts_.count(*active_host_) == 0;
  const bool new_state = !currently_enabled;
  if (new_state) {
    disabled_hosts_.erase(*active_host_);
  } else {
    disabled_hosts_.insert(*active_host_);
  }
  // Persist to database.
  if (db_ && db_->IsOpen()) {
    db_->SetShieldsEnabled(*active_host_, new_state);
  }
  return true;
}

}  // namespace kingfn
