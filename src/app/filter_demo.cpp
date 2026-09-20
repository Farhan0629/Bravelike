#include "core/filter_engine.h"
#include "core/privacy_stats.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage: bravelike_filter_demo <filter-list>\n";
    return 2;
  }
  bravelike::FilterEngine engine;
  try {
    const auto loaded = engine.LoadFromFile(argv[1]);
    std::cout << "Loaded " << loaded.block_rules << " block and " << loaded.allow_rules
              << " allow rules. Enter a URL, or Ctrl+Z/Ctrl+D to finish.\n";
    for (const auto& warning : loaded.warnings) std::cerr << "Warning: " << warning << '\n';
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }

  bravelike::PrivacyStats stats;
  std::string url;
  while (std::getline(std::cin, url)) {
    const auto decision = engine.Evaluate(url);
    const bool blocked = decision.action == bravelike::FilterAction::kBlock;
    stats.Record(blocked);
    std::cout << (blocked ? "BLOCK" : "ALLOW") << " | " << decision.reason;
    if (!decision.matched_rule.empty()) std::cout << " | " << decision.matched_rule;
    std::cout << '\n';
  }
  const auto snapshot = stats.Snapshot();
  std::cout << "Evaluated: " << snapshot.evaluated << ", blocked: " << snapshot.blocked << '\n';
}
