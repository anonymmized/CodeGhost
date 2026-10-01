#pragma once

#include "types.hpp"
#include "Settings/config.hpp"

#include <vector>
#include <filesystem>

class Scanner {
    private:
        bool recursive;
        std::vector<std::filesystem::path> ignorePaths;
        ScanPoint scanDir(const std::filesystem::path& directory);
        FileRecord scanFile(const std::filesystem::path& file);
        bool shouldIgnore(const std::filesystem::path& path) const;
    public:
        explicit Scanner(const Config& config) : recursive(config.recursive), ignorePaths(config.ignorePaths) {}
        ScanPoint scan(const std::vector<std::system::path>& paths);
};
