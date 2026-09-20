#pragma once
#include <atomic>
#include <cstdint>

namespace bravelike {
struct PrivacySnapshot { std::uint64_t evaluated; std::uint64_t blocked; };
class PrivacyStats {
 public:
  void Record(bool blocked);
  PrivacySnapshot Snapshot() const;
  void Reset();
 private:
  std::atomic<std::uint64_t> evaluated_{0};
  std::atomic<std::uint64_t> blocked_{0};
};
}  // namespace bravelike
