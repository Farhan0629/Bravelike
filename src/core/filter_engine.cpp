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
    return std::isalnum(c) || c == '.' || c == '-' || c == '_';
  });
}

bool IsPlausiblePath(std::string_view path) {
  if (path.empty() || path.front() != '/') return false;
  return std::none_of(path.begin(), path.end(), [](unsigned char c) {
    return std::isspace(c) || c == '*' || c == '^' || c == '|';
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
    if (rule.empty() || rule.front() == '#' || rule.front() == '!' || rule.front() == '[') continue;
    // Cosmetic rules are handled by the in-page script, not the network layer.
    if (rule.find("##") != std::string::npos || rule.find("#@#") != std::string::npos) continue;

    // Hosts-file format: "0.0.0.0 domain" or "127.0.0.1 domain".
    if (rule.rfind("0.0.0.0", 0) == 0 || rule.rfind("127.0.0.1", 0) == 0) {
      std::istringstream parts(rule);
      std::string ip, host;
      parts >> ip >> host;
      if (host == "localhost" || host == "0.0.0.0") continue;
      rule = host;
    }

    bool allow = false;
    if (rule.rfind("@@", 0) == 0) {
      allow = true;
      rule.erase(0, 2);
    }
    const auto options = rule.find('$');
    if (options != std::string::npos) rule.erase(options);
    if (rule.rfind("||", 0) == 0) rule.erase(0, 2);
    while (!rule.empty() && (rule.back() == '^' || rule.back() == '|')) rule.pop_back();
    rule = ToLowerAscii(Trim(rule));

    std::string domain = rule;
    std::string path;
    const auto slash = rule.find('/');
    if (slash != std::string::npos) {
      domain = rule.substr(0, slash);
      path = rule.substr(slash);
    }

    if (!IsPlausibleDomain(domain) || (!path.empty() && !IsPlausiblePath(path))) {
      result.warnings.push_back("Line " + std::to_string(line_number) + ": unsupported rule");
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
  // Walk host suffixes: a.b.example.com -> b.example.com -> example.com -> com
  std::string_view candidate = host;
  while (!candidate.empty()) {
    const auto it = rules.find(std::string(candidate));
    if (it != rules.end()) {
      for (const auto& rule : it->second) {
        if (rule.path_prefix.empty() || path.rfind(rule.path_prefix, 0) == 0) return &rule;
      }
    }
    const auto dot = candidate.find('.');
    if (dot == std::string_view::npos) break;
    candidate.remove_prefix(dot + 1);
  }
  return nullptr;
}

FilterDecision FilterEngine::Evaluate(std::string_view url) const {
  const auto host = ExtractHttpHost(url);
  if (!host) return {FilterAction::kAllow, {}, "unsupported or invalid URL"};
  const std::string path = ExtractPath(url);

  if (const auto* rule = Match(allow_rules_, *host, path))
    return {FilterAction::kAllow, rule->source, "matched allow rule"};
  if (const auto* rule = Match(block_rules_, *host, path))
    return {FilterAction::kBlock, rule->source, "matched block rule"};
  return {FilterAction::kAllow, {}, "no matching rule"};
}

std::size_t FilterEngine::rule_count() const {
  return block_count_ + allow_count_;
}
}  // namespace kingfn
