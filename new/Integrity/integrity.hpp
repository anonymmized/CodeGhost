#pragma once 

#include "Inspector/types.hpp"

#include <filesystem>
#include <vector>

enum class ChangeType {
    Added,
    Removed,
    Modified
};

struct FileChange {
    std::filesystem::path path;
    ChangeType type;

    bool contentChanged = false;
    bool sizeChanged = false;
    bool permissionsChanged = false;
    bool modificationTimeChanged = false;
};

std::vector<FileChange> compareBaseline(const std::vector<FileRecord>& baseline, const ScanPoint& currentScan);
