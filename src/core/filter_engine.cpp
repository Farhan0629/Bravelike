#include "core/filter_engine.h"
#include "core/url_utils.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace kingfn {
namespace {
std::string Trim(std::string_view value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string_view::npos) return {};
  const auto last = value.find_last_not_of(" \t\r\n");
  return std::string(value.substr(first, last - first + 1));
}

bool IsPlausibleDomain(std::string_view domain) {
  if (domain.empty() || domain.front() == '.' || domain.back() == '.') return false;
  if (domain.find('.') == std::string_view::npos) return false;
  return std::all_of(domain.begin(), domain.end(), [](unsigned char c) {
    return std::isalnum(c) || c == '.' || c == '-' || c == '_' || c == '*';
  });
}

bool IsPlausiblePath(std::string_view path) {
  if (path.empty() || path.front() != '/') return false;
  return std::none_of(path.begin(), path.end(), [](unsigned char c) {
    return std::isspace(c) || c == '^' || c == '|';
  });
}

// Returns the URL path (without query/fragment), lower-cased. "/" if absent.
std::string ExtractPath(std::string_view url) {
  const auto scheme_end = url.find("://");
  if (scheme_end == std::string_view::npos) return "/";
  auto rest = url.substr(scheme_end + 3);
  const auto slash = rest.find_first_of("/?#");
  if (slash == std::string_view::npos || rest[slash] != '/') return "/";
  rest = rest.substr(slash);
  const auto end = rest.find_first_of("?#");
  return ToLowerAscii(rest.substr(0, end));
}

// Simple wildcard match: pattern may contain '*' (matches any substring).
bool WildcardMatch(std::string_view pattern, std::string_view text) {
  if (pattern.empty()) return true;
  if (pattern == "*") return true;

  // Split pattern by '*' and greedily match substrings.
  std::vector<std::string_view> parts;
  std::size_t start = 0;
  while (true) {
    const auto pos = pattern.find('*', start);
    parts.push_back(pattern.substr(start, pos == std::string_view::npos ? pos : pos - start));
    if (pos == std::string_view::npos) break;
    start = pos + 1;
  }

  std::size_t search_from = 0;
  bool first = true;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    const auto& part = parts[i];
    if (part.empty()) {
      if (i == 0) first = false;
      continue;
    }
    const auto found = text.find(part, search_from);
    if (found == std::string_view::npos) return false;
    if (first && i == 0 && found != 0) return false;  // anchored start
    search_from = found + part.size();
    first = false;
  }

  // If last segment is non-empty, it must match up to end.
  if (!parts.empty() && !parts.back().empty()) {
    if (pattern.back() != '*') {
      // The last part must be a suffix match.
      return text.size() >= parts.back().size() &&
             text.substr(text.size() - parts.back().size()) == parts.back();
    }
  }
  return true;
}
}  // namespace

LoadResult FilterEngine::LoadFromFile(const std::string& path) {
  std::ifstream file(path);
  if (!file) throw std::runtime_error("Could not open filter list: " + path);
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return LoadFromText(buffer.str());
}

LoadResult FilterEngine::LoadFromText(std::string_view text) {
  block_rules_.clear();
  allow_rules_.clear();
  wildcard_block_rules_.clear();
  wildcard_allow_rules_.clear();
  block_count_ = 0;
  allow_count_ = 0;
  return AppendFromText(text);
}

LoadResult FilterEngine::AppendFromText(std::string_view text) {
  LoadResult result;
  std::istringstream stream{std::string(text)};
  std::string line;
  std::size_t line_number = 0;
  while (std::getline(stream, line)) {
    ++line_number;
    const std::string source = Trim(line);
    std::string rule = source;
    if (rule.empty() || rule.front() == '#' || rule.front() == '!' ||
        rule.front() == '[') continue;
    // Cosmetic/element-hide rules — handled by the in-page JS, not C++.
    if (rule.find("##") != std::string::npos ||
        rule.find("#@#") != std::string::npos ||
        rule.find("#?#") != std::string::npos) continue;

    // Hosts-file format: "0.0.0.0 domain" or "127.0.0.1 domain".
    if (rule.rfind("0.0.0.0", 0) == 0 || rule.rfind("127.0.0.1", 0) == 0) {
      std::istringstream parts(rule);
      std::string ip, host;
      parts >> ip >> host;
      if (host.empty() || host == "localhost" || host == "0.0.0.0") continue;
      rule = host;
    }

    bool allow = false;
    if (rule.rfind("@@", 0) == 0) {
      allow = true;
      rule.erase(0, 2);
    }

    // Strip options after '$' — split on first '$', discard everything after.
    const auto options_pos = rule.find('$');
    if (options_pos != std::string::npos) rule.erase(options_pos);

    // Strip Adblock-style anchors.
    if (rule.rfind("||", 0) == 0) rule.erase(0, 2);
    while (!rule.empty() && (rule.back() == '^' || rule.back() == '|'))
      rule.pop_back();

    rule = ToLowerAscii(Trim(rule));
    if (rule.empty()) continue;

    std::string domain = rule;
    std::string path;
    const auto slash = rule.find('/');
    if (slash != std::string::npos) {
      domain = rule.substr(0, slash);
      path = rule.substr(slash);
    }

    const bool has_domain_wildcard = domain.find('*') != std::string::npos;
    const bool has_path_wildcard   = path.find('*')   != std::string::npos;

    if (has_domain_wildcard) {
      // Wildcard domain rule — stored separately.
      WildcardRule wr{domain, path, source};
      if (allow) {
        wildcard_allow_rules_.push_back(std::move(wr));
      } else {
        wildcard_block_rules_.push_back(std::move(wr));
      }
      allow ? ++allow_count_ : ++block_count_;
      allow ? ++result.allow_rules : ++result.block_rules;
      continue;
    }

    if (!IsPlausibleDomain(domain) ||
        (!path.empty() && !has_path_wildcard && !IsPlausiblePath(path))) {
      result.warnings.push_back("Line " + std::to_string(line_number) +
                                 ": unsupported rule: " + source);
      continue;
    }

    Rule parsed{path, source};
    if (allow) {
      allow_rules_[domain].push_back(std::move(parsed));
      ++allow_count_;
      ++result.allow_rules;
    } else {
      block_rules_[domain].push_back(std::move(parsed));
      ++block_count_;
      ++result.block_rules;
    }
  }
  return result;
}

const FilterEngine::Rule* FilterEngine::Match(const RuleMap& rules,
                                              std::string_view host,
                                              std::string_view path) {
  if (rules.empty()) return nullptr;
  // Walk host suffixes: a.b.example.com → b.example.com → example.com → com
  std::string_view candidate = host;
  while (!candidate.empty()) {
    const auto it = rules.find(std::string(candidate));
    if (it != rules.end()) {
      for (const auto& rule : it->second) {
        if (rule.path_prefix.empty()) return &rule;
        // Path matching: support wildcards in path prefix.
        if (rule.path_prefix.find('*') != std::string::npos) {
          if (WildcardMatch(rule.path_prefix, path)) return &rule;
        } else {
          if (path.rfind(rule.path_prefix, 0) == 0) return &rule;
        }
      }
    }
    const auto dot = candidate.find('.');
    if (dot == std::string_view::npos) break;
    candidate.remove_prefix(dot + 1);
  }
  return nullptr;
}

bool FilterEngine::MatchWildcard(const std::vector<WildcardRule>& rules,
                                 std::string_view host,
                                 std::string_view path) {
  for (const auto& wr : rules) {
    if (WildcardMatch(wr.domain_pattern, host)) {
      if (wr.path_prefix.empty() || WildcardMatch(wr.path_prefix, path))
        return true;
    }
  }
  return false;
}

FilterDecision FilterEngine::Evaluate(std::string_view url) const {
  const auto host = ExtractHttpHost(url);
  if (!host) return {FilterAction::kAllow, {}, "unsupported or invalid URL"};
  const std::string path = ExtractPath(url);

  // Allow rules win unconditionally.
  if (const auto* rule = Match(allow_rules_, *host, path))
    return {FilterAction::kAllow, rule->source, "matched allow rule"};
  if (MatchWildcard(wildcard_allow_rules_, *host, path))
    return {FilterAction::kAllow, {}, "matched wildcard allow rule"};

  if (const auto* rule = Match(block_rules_, *host, path))
    return {FilterAction::kBlock, rule->source, "matched block rule"};
  if (MatchWildcard(wildcard_block_rules_, *host, path))
    return {FilterAction::kBlock, {}, "matched wildcard block rule"};

  return {FilterAction::kAllow, {}, "no matching rule"};
}

std::size_t FilterEngine::rule_count() const {
  return block_count_ + allow_count_;
}
}  // namespace kingfn
