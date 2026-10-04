#include "Inspector/scanner.hpp"
#include "Inspector/hasher.hpp"

#include <filesystem>
#include <utility>
#include <stdexcept>
#include <system_error>

namespace {
    std::filesystem::path makeAbsolute(const std::filesystem::path& pathToAbs) {
        if (pathToAbs.is_absolute()) {
            return pathToAbs;
        }
        return std::filesystem::absolute(pathToAbs);
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
    std::error_code error;
    std::filesystem::recursive_directory_iterator it{absolutePath, error};
    const std::filesystem::recursive_directory_iterator end;
    if (error) {
        scanPoint.errors.push_back({absolutePath, error.message()});
        return scanPoint;
    }
    while (it != end) {
        const auto entryPath = it->path();
        try {
            const bool needToIgnore = shouldIgnore(entryPath);
            if (!recursive || needToIgnore) {
                it.disable_recursion_pending();
            }
            if (!needToIgnore) {
                if (auto record = process(*it)) {
                    scanPoint.files.push_back(std::move(*record));
                }
            }
        } catch (const std::runtime_error& exception) {
            it.disable_recursion_pending();
            scanPoint.errors.push_back({entryPath, exception.what()});
        }
        it.increment(error);
        if (error) {
            scanPoint.errors.push_back({entryPath, "Directory traversal failed: " + error.message()});
            break;
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
    ScanPoint result;
    for (const auto& inputPath : inputPaths) {
        auto path = inputPath;
        try {
            path = makeAbsolute(inputPath).lexically_normal();
            if (shouldIgnore(path)) {
                continue;
            }
            const auto status = std::filesystem::symlink_status(path);
            if (std::filesystem::is_symlink(status)) {
                continue;
            }
            if (std::filesystem::is_regular_file(status)) {
                result.files.push_back(scanFile(path));
            } else if (std::filesystem::is_directory(status)) {
                auto directoryResult = scanDir(path);
                for (auto& file : directoryResult.files) {
                    result.files.push_back(std::move(file));
                }
                for (auto& error : directoryResult.errors) {
                    result.errors.push_back(std::move(error));
                }
            } else {
                result.errors.push_back({path, "Path does not exist or has an unsupported file type"});
            }
        } catch (const std::runtime_error& exception) {
            result.errors.push_back({path, exception.what()});
        }
    }
    return result;
}

bool Scanner::shouldIgnore(const std::filesystem::path& absolutePath) const {
    const auto checkedPath = absolutePath.lexically_normal();
    for (const auto& ignore : ignorePaths) {
        const auto ignoredPath = makeAbsolute(ignore).lexically_normal();
        auto current = checkedPath.begin();
        bool matches = true;
        for (const auto& component : ignoredPath) {
            if (component.empty()) {
                continue;
            }
            if (current == checkedPath.end() || *current != component) {
                matches = false;
                break;
            }
            ++current;
        }
        if (matches) {
            return true;
        }
    }
    return false;
}
