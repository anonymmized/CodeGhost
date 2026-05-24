#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "daemon.hpp"
#include "logger.hpp"

struct FileState {
    uint64_t hash = 0;
    uint64_t size = 0;
    uint32_t permissions = 0;
    int64_t write_time_ticks = 0;
};

class Hasher {
private:
    // baseline is the last approved trusted snapshot persisted on disk.
    // table is the latest observed runtime snapshot rebuilt from the filesystem and events.
    std::unordered_map<std::string, FileState> table;
    std::unordered_map<std::string, FileState> baseline;
    std::vector<std::string> ignore_paths;
    std::vector<std::string> critical_paths;
    bool recursive = true;

    std::optional<FileState> readFileState(const std::string& path);
    void recordFileState(const std::string& path);

public:
    Hasher(const std::vector<std::string>& _ignore_paths,
           const std::vector<std::string>& _critical_paths,
           bool _recursive)
        : ignore_paths(_ignore_paths), critical_paths(_critical_paths), recursive(_recursive) {}

    uint64_t calcHash(const std::string& path);
    bool isCriticalPath(const std::filesystem::path& path);
    void loadBaselineFile(const std::string& path);
    bool shouldIgnoreDir(const std::filesystem::path& path);
    void processFileEntry(const std::filesystem::directory_entry& entry);
    void calcDirHashes(const std::string& current_path);
    void loadBaseline(const std::string& path);
    void initHashes(const Config& conf);
    void saveBaseline(const std::string& baseline_path);
    void syncBaseline(const std::string& path);
    void reloadRuntime(const Config& conf);
    void resetRuntimeToBaseline();
    LogLevel levelForPath(const std::filesystem::path& path, LogLevel critical_level);
    void deleteHash(const std::string& path, Logger& logger);
    void deletePathTree(const std::string& path, Logger& logger);
    void fileChanged(const std::string& path, Logger& logger);
    void fileAttributed(const std::string& path, Logger& logger);
    void registerPathTree(const std::string& path, Logger& logger);
    void movePathTree(const std::string& old_path, const std::string& new_path, Logger& logger, bool is_directory);
};
