#pragma once
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace bravelike {
enum class FilterAction { kAllow, kBlock };

struct FilterDecision {
  FilterAction action{FilterAction::kAllow};
  std::string matched_rule;
  std::string reason;
};

struct LoadResult {
  std::size_t block_rules{0};
  std::size_t allow_rules{0};
  std::vector<std::string> warnings;
};

class FilterEngine {
 public:
  LoadResult LoadFromFile(const std::string& path);
  LoadResult LoadFromText(std::string_view text);
  FilterDecision Evaluate(std::string_view url) const;
  std::size_t rule_count() const;

 private:
  struct Rule { std::string domain; std::string source; };
  std::vector<Rule> block_rules_;
  std::vector<Rule> allow_rules_;
};
}  // namespace bravelike
