#include "Inspector/scanner.hpp"
#include "Inspector/hasher.hpp"

#include <filesystem>
#include <utility>

namespace {
    std::filesystem::path makeAbsolute(const std::filesystem::path& pathToAbs) {
        if (pathToAbs.is_absolute()) {
            return pathToAbs;
        }
        return std::filesystem::absolute(pathToAbs);
    }
    std::vector<std::filesystem::path> checkAbsolutePaths(const std::vector<std::filesystem::path>& paths) {
        std::vector<std::filesystem::path> absPaths;
        for (const auto& path : paths) {
            absPaths.push_back(makeAbsolute(path));
        }
        return absPaths;
    }
};

std::optional<FileRecord> Scanner::process(const std::filesystem::directory_entry& entry) {
    if (entry.is_symlink() || !entry.is_regular_file()) {
        return std::nullopt;
    }
    return scanFile(entry.path());
}

ScanPoint Scanner::scanDir(const std::filesystem::path& absolutePath) {
    ScanPoint scanPoint;

    if (recursive) {
        for (const auto& dir_entry : std::filesystem::recursive_directory_iterator{absolutePath}) {
            if (auto record = process(dir_entry)) {
                scanPoint.files.push_back(std::move(*record));
            }
        }
    } else {
        for (const auto& dir_entry : std::filesystem::directory_iterator{absolutePath}) {
            if (auto record = process(dir_entry)) {
                scanPoint.files.push_back(std::move(*record));
            }
        }
    }
    return scanPoint;
}

FileRecord Scanner::scanFile(const std::filesystem::path& absolutePath) {
    FileRecord fileRecord;
    fileRecord.path = absolutePath;
    fileRecord.contentHash = HashSession::hashFile(absolutePath);
    fileRecord.size = std::filesystem::file_size(absolutePath);
    fileRecord.permissions = std::filesystem::status(absolutePath).permissions();
    fileRecord.modificationTime = std::filesystem::last_write_time(absolutePath);
    return fileRecord;
}

ScanPoint Scanner::scan(const std::vector<std::filesystem::path>& inputPaths) {
    auto checkedPaths = checkAbsolutePaths(inputPaths);
    ScanPoint result;
    for (const auto& path : checkedPaths) {
       if (std::filesystem::is_regular_file(path)) {
            result.files.push_back(scanFile(path));
       } else if (std::filesystem::is_directory(path)) {
            auto scanedDirPoint = scanDir(path);
            result.files.insert(result.files.end(), scanedDirPoint.files.begin(), scanedDirPoint.files.end());
            result.errors.insert(result.errors.end(), scanedDirPoint.errors.begin(), scanedDirPoint.errors.end());
       }
    }
    return result;
}

bool Scanner::shouldIgnore(const std::filesystem::path& absolutePath) const {

}
