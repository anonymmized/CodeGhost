#pragma once

#include <unordered_map>
#include <string>
#include <filesystem>
#include <vector>
#include <cstdint>

#include "logger.hpp"
#include "daemon.hpp"

struct MoveEvent {
    std::string old_path;
    uint64_t hash;
};

class Hasher {
    private:
        std::unordered_map<std::string, uint64_t> table;
        std::unordered_map<std::string, uint64_t> baseline;
        std::vector<std::string> ignore_paths;
        std::unordered_map<uint32_t, MoveEvent> move_buffer;
        bool recursive = true;
    public:
        Hasher(const std::vector<std::string>& _ignore_paths,
               bool _recursive) : ignore_paths(_ignore_paths), recursive(_recursive) {}
        uint64_t calcHash(const std::string& path); // calculate file's hash on 'path'
        void loadBaselineFile(const std::string& path); // load baseline in table in start
        bool compareHashes(const uint64_t& old_hash, const std::string& path); // compare two hashes
        bool shouldIgnoreDir(const std::filesystem::path& path); // check if dir need to be ignored
        void processFileEntry(const std::filesystem::directory_entry& entry); // helper method for code readability
        void calcDirHashes(const std::string& current_path); // recursive or not calculation of all files in target directory
        void loadBaseline(const std::string& path); // load all paths:hashes pairs to hashtable
        void initHashes(const Config& conf, const std::string& watch_path, const std::string& baseline_path); // upload new changes to baseline.json
        void deleteHash(const std::string& path, Logger& logger); // handle file deletion
        void fileChanged(const std::string& path, Logger& logger); // handle file editing or creating
        void fileMoved(const std::string& path, Logger& logger, bool moved, uint32_t& cookie); // handle IN_MOVED_FROM and IN_MOVED_TO
        void updateHash(const std::string& path, const uint64_t new_hash); // just update file's hash in table without erasing it
};
