#include "core/privacy_stats.h"
namespace bravelike {
void PrivacyStats::Record(bool blocked) {
  evaluated_.fetch_add(1, std::memory_order_relaxed);
  if (blocked) blocked_.fetch_add(1, std::memory_order_relaxed);
}
PrivacySnapshot PrivacyStats::Snapshot() const {
  return {evaluated_.load(std::memory_order_relaxed), blocked_.load(std::memory_order_relaxed)};
}
void PrivacyStats::Reset() {
  evaluated_.store(0, std::memory_order_relaxed);
  blocked_.store(0, std::memory_order_relaxed);
}
}  // namespace bravelike
