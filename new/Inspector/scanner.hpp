#pragma once

#include "types.hpp"
#include "Settings/config.hpp"

#include <vector>
#include <optional>
#include <filesystem>

class Scanner {
    private:
        bool recursive;
        std::vector<std::filesystem::path> ignorePaths;
        ScanPoint scanDir(const std::filesystem::path& absolutePath);
        FileRecord scanFile(const std::filesystem::path& absolutePath);
        bool shouldIgnore(const std::filesystem::path& absolutePath) const;
        std::optional<FileRecord> process(const std::filesystem::directory_entry& entry);
    public:
        explicit Scanner(const Config& config) : recursive(config.recursive), ignorePaths(config.ignorePaths) {}
        ScanPoint scan(const std::vector<std::filesystem::path>& inputPaths);
};
