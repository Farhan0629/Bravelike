#include "core/site_shields.h"

#include <iostream>
#include <string>

namespace {
int failures = 0;
void Expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}
}  // namespace

int main() {
  bravelike::SiteShields shields;
  Expect(!shields.ActiveHost() && !shields.EnabledForActive(),
         "empty page has no active Shields policy");
  Expect(!shields.ToggleActive(), "cannot toggle an empty page");
  shields.SetActiveUrl("HTTPS://Example.COM:443/a");
  Expect(shields.ActiveHost().value_or("") == "example.com",
         "normalizes active host");
  Expect(shields.EnabledForActive(), "Shields are on by default");
  Expect(shields.ToggleActive() && !shields.EnabledForActive(),
         "toggle disables the active site");
  shields.SetActiveUrl("https://other.example/path");
  Expect(shields.EnabledForActive(), "another site remains protected");
  shields.SetActiveUrl("https://example.com/again");
  Expect(!shields.EnabledForActive(), "disabled site keeps its own policy");
  Expect(shields.ToggleActive() && shields.EnabledForActive(),
         "toggle re-enables the active site");
  shields.SetActiveUrl("file:///tmp/page.html");
  Expect(!shields.ActiveHost() && !shields.ToggleActive(),
         "non-HTTP page has no site toggle");
  shields.SetActiveUrl("https://other.example/");
  Expect(shields.EnabledForActive(), "switching away does not change another site");
  if (failures == 0) std::cout << "All per-site Shields tests passed.\n";
  return failures == 0 ? 0 : 1;
}
