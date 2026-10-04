#include "Integrity/integrity.hpp"

#include <optional>
#include <algorithm>
#include <stdexcept>
#include <unordered_set>

namespace {
    std::optional<FileChange> compareFields(const FileRecord& baselineRecord, const FileRecord& currentRecord) {
        FileChange change{};
        change.path = currentRecord.path;
        change.type = ChangeType::Modified;
        change.contentChanged = baselineRecord.contentHash != currentRecord.contentHash;
        change.sizeChanged = baselineRecord.size != currentRecord.size;
        change.permissionsChanged = baselineRecord.permissions != currentRecord.permissions;
        change.modificationTimeChanged = baselineRecord.modificationTime != currentRecord.modificationTime;
        if (!change.contentChanged && !change.sizeChanged && !change.permissionsChanged && !change.modificationTimeChanged) {
            return std::nullopt;
        }
        return change;
    }

    const FileRecord* findByPath(const std::vector<FileRecord>& files, const std::filesystem::path& path) {
        const auto found = std::find_if(files.begin(), files.end(), [&path](const FileRecord& record){ return record.path == path; });
        return found == files.end() ? nullptr : &*found;
    }

    void validateUniquePaths(const std::vector<FileRecord>& files) {
        std::unordered_set<std::filesystem::path> seenPaths;
        for (const auto& file : files) {
            const auto [iterator, inserted] = seenPaths.insert(file.path);
            if (!inserted) {
                throw std::runtime_error("Duplicate file path: " + file.path.string());
            }
        }
    }
};

std::vector<FileChange> compareBaseline(const std::vector<FileRecord>& baseline, const ScanPoint& currentScan) {
    if (!currentScan.errors.empty()) {
        throw std::runtime_error("Cannot compare an incomplete scan");
    }
    validateUniquePaths(baseline);
    validateUniquePaths(currentScan.files);
    std::vector<FileChange> fileChange;

    for (const auto& current : currentScan.files) {
        const auto* previous = findByPath(baseline, current.path);
        if (!previous) {
            fileChange.push_back(FileChange{current.path, ChangeType::Added});
        } else if (auto change = compareFields(*previous, current)) {
            fileChange.push_back(*change);
        }
    }
    for (const auto& previous : baseline) {
        if (!findByPath(currentScan.files, previous.path)) {
            fileChange.push_back(FileChange{previous.path, ChangeType::Removed});
        }
    }
    std::sort(fileChange.begin(), fileChange.end(), [](const FileChange& left, const FileChange& right) { return left.path < right.path; });
    return fileChange;
}
