#pragma once
#include "include/cef_parser.h"
#include "src/core/home_policy.h"
namespace kingfn {
// Canonicalize through CEF so the saved home URL matches OnAddressChange.
inline CefString CanonicalHomeFileUrl(const CefString& path) {
  CefURLParts parts;
  if (!CefParseURL(WindowsPathToFileUrl(path.ToString()), parts)) return "about:blank";
  return CefString(&parts.spec);
}
}
