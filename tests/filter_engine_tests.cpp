#include "core/filter_engine.h"
#include "core/navigation.h"
#include "core/privacy_stats.h"
#include "core/url_utils.h"
#include <iostream>
#include <string>

namespace {
int failures = 0;
void Expect(bool condition, const std::string& message) {
  if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
}

int main() {
  using bravelike::FilterAction;
  bravelike::FilterEngine engine;
  const auto result = engine.LoadFromText(R"(
# example rules
ads.example.com
||tracker.example^
@@allowed.tracker.example
not a supported rule
)");
  Expect(result.block_rules == 2, "loads two block rules");
  Expect(result.allow_rules == 1, "loads one allow rule");
  Expect(result.warnings.size() == 1, "reports unsupported rules");
  Expect(engine.Evaluate("https://ads.example.com/banner.js").action == FilterAction::kBlock,
         "blocks an exact domain");
  Expect(engine.Evaluate("https://cdn.ads.example.com/banner.js").action == FilterAction::kBlock,
         "blocks a subdomain");
  Expect(engine.Evaluate("https://badads.example.com/").action == FilterAction::kAllow,
         "does not use unsafe suffix matching");
  Expect(engine.Evaluate("https://allowed.tracker.example/pixel").action == FilterAction::kAllow,
         "allow rule has precedence");
  Expect(engine.Evaluate("file:///tmp/page.html").action == FilterAction::kAllow,
         "allows unsupported schemes");
  Expect(bravelike::ExtractHttpHost("HTTPS://User:Pass@Example.COM:443/a").value_or("") == "example.com",
         "normalizes authority and host");

  Expect(bravelike::ResolveAddressInput("example.com") == "https://example.com",
         "adds HTTPS to a host");
  Expect(bravelike::ResolveAddressInput("http://localhost:8080") == "http://localhost:8080",
         "preserves explicit schemes");
  Expect(bravelike::ResolveAddressInput("privacy browser") ==
             "https://search.brave.com/search?q=privacy+browser",
         "turns words into a search query");
  Expect(bravelike::ResolveAddressInput(" C++ browser ") ==
             "https://search.brave.com/search?q=C%2B%2B+browser",
         "trims and encodes a search query");

  bravelike::PrivacyStats stats;
  stats.Record(true); stats.Record(false);
  const auto snapshot = stats.Snapshot();
  Expect(snapshot.evaluated == 2 && snapshot.blocked == 1, "records privacy statistics");

  if (failures == 0) std::cout << "All Bravelike core tests passed.\n";
  return failures == 0 ? 0 : 1;
}
