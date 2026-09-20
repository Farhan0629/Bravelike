#include "core/filter_engine.h"
#include "core/url_utils.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace bravelike {
namespace {
std::string Trim(std::string_view value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string_view::npos) return {};
  const auto last = value.find_last_not_of(" \t\r\n");
  return std::string(value.substr(first, last - first + 1));
}

bool IsPlausibleDomain(std::string_view domain) {
  if (domain.empty() || domain.front() == '.' || domain.back() == '.') return false;
  return std::all_of(domain.begin(), domain.end(), [](unsigned char c) {
    return std::isalnum(c) || c == '.' || c == '-';
  });
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
  LoadResult result;
  std::istringstream stream{std::string(text)};
  std::string line;
  std::size_t line_number = 0;
  while (std::getline(stream, line)) {
    ++line_number;
    auto rule = Trim(line);
    if (rule.empty() || rule.front() == '#' || rule.front() == '!') continue;

    bool allow = false;
    if (rule.rfind("@@", 0) == 0) {
      allow = true;
      rule.erase(0, 2);
    }
    if (rule.rfind("||", 0) == 0) rule.erase(0, 2);
    if (!rule.empty() && rule.back() == '^') rule.pop_back();
    rule = ToLowerAscii(Trim(rule));

    if (!IsPlausibleDomain(rule)) {
      result.warnings.push_back("Line " + std::to_string(line_number) + ": unsupported rule");
      continue;
    }
    Rule parsed{rule, Trim(line)};
    if (allow) {
      allow_rules_.push_back(std::move(parsed));
      ++result.allow_rules;
    } else {
      block_rules_.push_back(std::move(parsed));
      ++result.block_rules;
    }
  }
  return result;
}

FilterDecision FilterEngine::Evaluate(std::string_view url) const {
  const auto host = ExtractHttpHost(url);
  if (!host) return {FilterAction::kAllow, {}, "unsupported or invalid URL"};

  for (const auto& rule : allow_rules_) {
    if (IsDomainOrSubdomain(*host, rule.domain))
      return {FilterAction::kAllow, rule.source, "matched allow rule"};
  }
  for (const auto& rule : block_rules_) {
    if (IsDomainOrSubdomain(*host, rule.domain))
      return {FilterAction::kBlock, rule.source, "matched block rule"};
  }
  return {FilterAction::kAllow, {}, "no matching rule"};
}

std::size_t FilterEngine::rule_count() const {
  return block_rules_.size() + allow_rules_.size();
}
}  // namespace bravelike
