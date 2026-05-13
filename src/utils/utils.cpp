#include <string>
#include <vector>
#include <filesystem>

#include "../core/daemon.hpp"

bool shouldIgnoreTree(const std::filesystem::path& path, const std::vector<std::string>& ignore_paths) {
  std::string current = path.string();
  for (const auto& ignore : ignore_paths) {
    if (current.starts_with(ignore)) {
      return true;
    }
  }
  return false;
}

Config createDefaultConfig() {
    Config conf;
    conf.watch_paths = {"/tmp"};
    conf.ignore_paths = {};
    conf.critical_paths = {};

    conf.start_hour = 0;
    conf.end_hour = 23;

    conf.watch_recursive = true;

    return conf;
}
