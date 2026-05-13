#include <filesystem>
#include <vector>
#include <string>

#include "../core/daemon.hpp"

bool shouldIgnoreTree(const std::filesystem::path& path, const std::vector<std::string>& ignore_paths);
Config createDefaultConfig();
