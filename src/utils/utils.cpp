#include <string>
#include <vector>
#include <filesystem>

bool shouldIgnoreTree(const std::filesystem::path& path, const std::vector<std::string>& ignore_paths) {
  std::string current = path.string();
  for (const auto& ignore : ignore_paths) {
    if (current.starts_with(ignore)) {
      return true;
    }
  }
  return false;
}
