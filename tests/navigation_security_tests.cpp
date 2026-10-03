#include "core/home_policy.h"
#include "core/navigation.h"
#include <iostream>
int main() {
  int failures = 0;
  auto expect = [&](bool condition, const char* name) { if (!condition) { std::cerr << "FAIL: " << name << '\n'; ++failures; } };
  const char* home = "file:///C:/KINGFN/resources/home.html";
  expect(kingfn::IsExactHomeUrl(home, home), "exact owned home");
  expect(!kingfn::IsExactHomeUrl("https://evil.test/resources/home.html", home), "external lookalike");
  expect(!kingfn::IsExactHomeUrl("https://evil.test/?next=resources/home.html", home), "query lookalike");
  expect(!kingfn::IsExactHomeUrl("about:blank", home), "unrelated blank");
  expect(!kingfn::IsExactHomeUrl("", ""), "empty is not trusted");
  expect(kingfn::IsHomeAlias("kingfn://home"), "home alias");
  expect(!kingfn::IsHomeAlias("https://kingfn.test"), "external alias lookalike");
  expect(kingfn::WindowsPathToFileUrl("C:\\KING FN\\home#1.html") == "file:///C:/KING%20FN/home%231.html", "spaces and fragment escaped");
  expect(kingfn::WindowsPathToFileUrl("C:\\100%\\home.html") == "file:///C:/100%25/home.html", "percent escaped");
  expect(kingfn::ResolveAddressInput("youtube.com") == "https://youtube.com", "real-site hostname");
  expect(kingfn::ResolveAddressInput("https://youtube.com/watch?v=1") == "https://youtube.com/watch?v=1", "explicit URL");
  return failures ? 1 : 0;
}
