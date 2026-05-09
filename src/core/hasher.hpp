#pragma once

#include <unordered_map>
#include <string>

// calculate files hash
uint64_t calcHash(const std::string& path);

// to compare old hash and just calculated files hash
bool compareHashes(const uint64_t& old_hash, const std::string& path);

// check if directory is needed to be ignored
bool shouldIgnoreDir(const std::filesystem::path& path, const std::vector<std::string>& ignore_paths);

// helper function to improve code readability
void processFileEntry(std::unordered_map<std::string, uint64_t>& table, const std::filesystem::directory_entry& entry, const std::vector<std::string>& ignore_paths);

// calculate file hashes depends on recursive flag
void calcDirHashes(std::unordered_map<std::string, uint64_t>& table, const std::string& current_path, bool recursive, const std::vector<std::string>& ignore_paths);

// fill up a hashtable by baseline.json
std::unordered_map<std::string, uint64_t> loadBaseline(const std::string& path);

// to collect hashes to baseline.json
void initHashes(const Config& conf, const std::string& path);

class Hasher {
    private:
        std::unordered_map<std::string, uint64_t> table;
        std::vector<std::string> ignore_paths;
        bool recursive = true;
    public:
        Hasher(std::unordered_map<std::string, uint64_t>& _table,
               std::vector<std::string>& _ignore_paths,
               bool _recursive) : table(_table), ignore_paths(_ignore_paths), recursive(_recursive) {}
        uint64_t calcHash(const std::string& path);
        bool compareHashes(const uint64_t& old_hash, const std::string& path);
        bool shouldIgnoreDir(const std::filesystem::path& path);
        void processFileEntry(const std::filesystem::directory_entry& entry);
        void calcDirHashes(const std::string& current_path);
        std::unordered_map<std::string, uint64_t> loadBaseline(const std::string& path);
        void initHashes(const Config& conf, const std::string& path);
        void deleteHash(const std::string& path);
};
