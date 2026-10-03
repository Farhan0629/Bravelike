#pragma once
#include <functional>
#include <string>
#include <thread>

namespace kingfn {

// Semantic version comparison helper.
struct Version {
  int major{0}, minor{0}, patch{0};
  static Version Parse(const std::string& s);
  bool operator>(const Version& o) const;
  std::string ToString() const;
};

// Checks GitHub releases API for a newer version of KINGFN.
// All network work happens on a background thread; the callback
// is posted to the CEF UI thread when a result is available.
class UpdateChecker {
 public:
  // current_version: e.g. "0.3.0"
  // on_update_available: called with the new version string when found.
  //                      Called on the CEF UI thread.
  UpdateChecker(std::string current_version,
                std::function<void(std::string new_version,
                                   std::string release_url)> on_update_available);
  ~UpdateChecker();

  // Starts the background check. Safe to call only once.
  void CheckAsync();

 private:
  void DoCheck();
  static std::string FetchLatestRelease();

  std::string current_version_;
  std::function<void(std::string, std::string)> callback_;
  std::thread thread_;
};

}  // namespace kingfn
