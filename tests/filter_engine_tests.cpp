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
  using kingfn::FilterAction;
  kingfn::FilterEngine engine;
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
  Expect(kingfn::ExtractHttpHost("HTTPS://User:Pass@Example.COM:443/a").value_or("") == "example.com",
         "normalizes authority and host");

  Expect(kingfn::ResolveAddressInput("example.com") == "https://example.com",
         "adds HTTPS to a host");
  Expect(kingfn::ResolveAddressInput("http://localhost:8080") == "http://localhost:8080",
         "preserves explicit schemes");
  Expect(kingfn::ResolveAddressInput("privacy browser") ==
             "https://duckduckgo.com/?q=privacy+browser",
         "turns words into a search query");
  Expect(kingfn::ResolveAddressInput(" C++ browser ") ==
             "https://duckduckgo.com/?q=C%2B%2B+browser",
         "trims and encodes a search query");

  kingfn::PrivacyStats stats;
  stats.Record(true); stats.Record(false);
  const auto snapshot = stats.Snapshot();
  Expect(snapshot.evaluated == 2 && snapshot.blocked == 1, "records privacy statistics");

  // Real-world rule syntax used by the shipped blocklist.
  kingfn::FilterEngine real;
  const auto loaded = real.LoadFromText(R"(
doubleclick.net
0.0.0.0 tracker.net
||adnxs.com^$third-party
||youtube.com/api/stats/ads
||youtube.com/pagead/
example.com##.ad-banner
)");
  Expect(loaded.block_rules == 5 && loaded.warnings.empty(), "parses real-world syntax");
  Expect(real.Evaluate("https://googleads.g.doubleclick.net/pagead/id").action == FilterAction::kBlock,
         "blocks doubleclick subdomain");
  Expect(real.Evaluate("https://cdn.tracker.net/p.gif").action == FilterAction::kBlock,
         "hosts-file rule blocks");
  Expect(real.Evaluate("https://ib.adnxs.com/ut/v3").action == FilterAction::kBlock,
         "options are ignored");
  Expect(real.Evaluate("https://www.youtube.com/api/stats/ads?ver=2").action == FilterAction::kBlock,
         "blocks YouTube ad stats path");
  Expect(real.Evaluate("https://www.youtube.com/pagead/viewthroughconversion/1").action == FilterAction::kBlock,
         "blocks YouTube pagead path");
  Expect(real.Evaluate("https://www.youtube.com/watch?v=abc").action == FilterAction::kAllow,
         "does not block YouTube videos");
  Expect(real.Evaluate("https://www.youtube.com/youtubei/v1/player").action == FilterAction::kAllow,
         "does not block YouTube player API");

  if (failures == 0) std::cout << "All KINGFN core tests passed.\n";
  return failures == 0 ? 0 : 1;
}
