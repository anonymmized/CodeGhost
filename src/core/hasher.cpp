//#include <xxhash.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <xxhash.h>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "daemon.hpp"

using json = nlohmann::ordered_json;

uint64_t calcHash(const std::string& path) {
    std::ifstream infile(path, std::ios::binary);
    if (!infile.is_open()) {
        throw std::runtime_error("File wasn't opened");
    }

    XXH64_state_t* state = XXH64_createState();
    if (!state) {
        throw std::runtime_error("Failed to allocate state");
    }
    XXH64_reset(state, 0);

    char buff[8192];
    while (infile.read(buff, sizeof(buff)) || infile.gcount() > 0) {
        XXH64_update(state, buff, infile.gcount());
    }

    uint64_t hash = XXH64_digest(state);
    XXH64_freeState(state);

    return hash;
}

bool compareHashes(const uint64_t& old_hash, const std::string& path) {
    return calcHash(path) == old_hash;
}

bool shouldIgnoreDir(const std::filesystem::path& path, const std::vector<std::string>& ignore_paths) {
    std::string current = path.string();
    for (const auto& ignore : ignore_paths) {
        if (current.starts_with(ignore)) return true;
    }
    return false;
}

// helper to doublecode sectors
void processFileEntry(std::unordered_map<std::string, uint64_t>& table,
                      const std::filesystem::directory_entry& entry,
                      const std::vector<std::string>& ignore_paths) {
    try {
        if (!std::filesystem::is_regular_file(entry.path())) return;
        if (shouldIgnoreDir(entry.path(), ignore_paths)) return;
        std::string wfile = entry.path().string();
        table[wfile] = calcHash(wfile);
    } catch (std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << '\n';
    } catch (std::exception& e) {
        std::cerr << "Error processing file " << entry.path() << ": " << e.what() << '\n';
    }
}

void calcDirHashes(std::unordered_map<std::string, uint64_t>& table,
                   const std::string& current_path,
                   bool recursive,
                   const std::vector<std::string>& ignore_paths) {
    if (!std::filesystem::exists(current_path))
        throw std::runtime_error("Path doesn't exist: " + current_path);
    if (!std::filesystem::is_directory(current_path))
        throw std::runtime_error("Path isn't directory: " + current_path);
    if (recursive) {
        for (const auto& file : std::filesystem::recursive_directory_iterator(current_path)) {
            processFileEntry(table, file, ignore_paths);
        }
    } else {
        for (const auto& file : std::filesystem::directory_iterator(current_path)) {
            processFileEntry(table, file, ignore_paths);
        }
    }
}

std::unordered_map<std::string, uint64_t> loadBaseline(const std::string& path) {
    std::unordered_map<std::string, uint64_t> table;
    std::ifstream baseline(path);
    if (!baseline.is_open())
        throw std::runtime_error("The baseline.json wasn't opened");
    json j;
    baseline >> j;
    for (const auto& pair : j.items()) {
        table[pair.key()] = pair.value().get<uint64_t>();
    }
    return table;
}

void initHashes(const Config& conf, const std::string& path) {
    std::unordered_map<std::string, uint64_t> table;
    calcDirHashes(table, path, conf.watch_recursive);
    std::ofstream baseline("baseline.json");
    if (!baseline.is_open()) {
        throw std::runtime_error("The baseline.json wasn't opened");
    }
    json j;
    for (const auto& pair : table) {
        j[pair.first] = pair.second;
    }
    baseline << j.dump(4);
    baseline.close();
}

