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
