#pragma once
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace kingfn {
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

// Supported rule syntax (one per line):
//   example.com                  block domain and all subdomains
//   ||example.com^               same, Adblock-style anchor
//   ||example.com/path/          block only when URL path starts with /path/
//   ||example.com/prefix*        block path using wildcard
//   *.example.com                wildcard domain block
//   @@example.com                allow rule (wins over any block)
//   0.0.0.0 example.com          hosts-file format (127.0.0.1 also accepted)
//   ||example.com^$third-party   options after '$' are stripped and ignored
// Lines starting with '#', '!', or '[' are comments.
// Cosmetic rules (##, #@#, #?#) are skipped (handled by shields.js).
class FilterEngine {
 public:
  LoadResult LoadFromFile(const std::string& path);
  LoadResult LoadFromText(std::string_view text);
  // Adds rules without clearing previously loaded ones.
  LoadResult AppendFromText(std::string_view text);
  FilterDecision Evaluate(std::string_view url) const;
  std::size_t rule_count() const;

 private:
  struct Rule {
    std::string path_prefix;  // empty = whole domain; may contain '*'
    std::string source;
  };
  struct WildcardRule {
    std::string domain_pattern;  // contains '*'
    std::string path_prefix;
    std::string source;
  };

  using RuleMap = std::unordered_map<std::string, std::vector<Rule>>;

  static const Rule* Match(const RuleMap& rules, std::string_view host,
                           std::string_view path);
  static bool MatchWildcard(const std::vector<WildcardRule>& rules,
                            std::string_view host, std::string_view path);

  RuleMap block_rules_;
  RuleMap allow_rules_;
  std::vector<WildcardRule> wildcard_block_rules_;
  std::vector<WildcardRule> wildcard_allow_rules_;
  std::size_t block_count_{0};
  std::size_t allow_count_{0};
};
}  // namespace kingfn
