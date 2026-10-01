#pragma once

struct FileRecord {
    std::filesystem::path path;
    std::string contentHash;
    std::uintmax_t size;
    std::filesystem::perms permissions;
    std::filesystem::file_time_type modificationTime;
};

struct ScanError {
    std::filesystem::path path;
    std::string message;
};

struct ScanPoint {
    std::vector<FileRecord> files;
    std::vector<ScanError> errors;
};
