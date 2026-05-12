#include <filesystem>
#include <vector>
#include <string>

bool shouldIgnoreTree(const std::filesystem::path& path, const std::vector<std::string>& ignore_paths);
