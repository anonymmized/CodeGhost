#include <fstream>
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <xxhash.h>
#include <unordered_map>
#include <nlohmann/json.hpp>

#include "hasher.hpp"

using json = nlohmann::ordered_json;

/*
 * [ baseline in Hasher class means a basic filenames and hashes in it ]
 * [ table is a current system state. it changes until admin accept changes and then baseline updates with table ]
 * [ ignore_paths and recursion variables are using for better code readability ]
*/

/* this function is used to check changed metadata parts
void Hasher::fileAttributed(const std::string& path, Logger& logger) {
    if (!std::filesystem::exists(path)) return;
    uint64_t check_hash = calcHash(path);

    auto it = baseline.find(path);
    if (it != baseline.end()) {
        uint64_t old_hash = it->second;
        if (old_hash != check_hash) {
            table[path] = check_hash;
            logger.log(LOG_INFO, "File's attribute and content were changed: " + path);
        } else {
            logger.log(LOG_INFO, "File's attribute were changed: " + path);
        }
    }
}
*/

void Hasher::updateHash(const std::string& path, const uint64_t new_hash) {
    auto it = table.find(path);
    if (it != table.end()) {
        it->second = new_hash;
    } else {
        table[path] = new_hash;
    }
}
// 'MoveEvent' saves previous changes like IN_MOVED_TO and IN_MOVED_FROM to undrestand what is going on with file
// 'moved' flag handles event: IN_MOVED_FROM - false | IN_MOVED_TO - true. It used to understand what action need to handle
// 'cookie' is a unique identifier for IN_MOVED_TO and IN_MOVED_FROM pair
void Hasher::fileMoved(const std::string& path, Logger& logger, bool moved, uint32_t& cookie) {
    if (!moved) {
        MoveEvent tempEvent; //
        tempEvent.old_path = path;
	if (std::filesystem::exists(path))
            tempEvent.hash = calcHash(path);
        move_buffer[cookie] = tempEvent;
    } else {
        auto it = move_buffer.find(cookie);
        if (it != move_buffer.end()) {
            std::string old_path = it->second.old_path;
            uint64_t old_hash = it->second.hash;
            uint64_t new_hash = calcHash(path);

            deleteHash(old_path, logger);
            updateHash(path, new_hash);
            logger.log(LOG_INFO, "Moved:" + old_path + " > " + path);
            move_buffer.erase(it);
        } else {
            fileChanged(path, logger);
        }
    }
}

void Hasher::fileChanged(const std::string& path, Logger& logger) {
    if (!std::filesystem::exists(path)) return;
    uint64_t new_hash = calcHash(path);

    auto it = baseline.find(path);
    if (it == baseline.end()) {
        table[path] = new_hash;
        logger.log(LOG_INFO, "Created: " + path);
    } else if (it->second != new_hash) {
        table[path] = new_hash;
        logger.log(LOG_INFO, "Modified: " + path);
    }
}

uint64_t Hasher::calcHash(const std::string& path) {
    std::ifstream infile(path, std::ios::binary);
    if (!infile.is_open())
        throw std::runtime_error("File wasn't opened");
    XXH64_state_t* state = XXH64_createState();
    if (!state)
        throw std::runtime_error("Failed to allocate state");
    XXH64_reset(state, 0);
    char buff[8192];
    while (infile.read(buff, sizeof(buff)) || infile.gcount() > 0) {
        XXH64_update(state, buff, infile.gcount());
    }
    uint64_t hash = XXH64_digest(state);
    XXH64_freeState(state);
    return hash;
}

void Hasher::loadBaselineFile(const std::string& path) {
    loadBaseline(path);
    table = baseline;
}

bool Hasher::compareHashes(const uint64_t& old_hash, const std::string& path) {
    return calcHash(path) == old_hash;
}

bool Hasher::shouldIgnoreDir(const std::filesystem::path& path) {
    std::string current = path.string();
    for (const auto& ignore : ignore_paths) {
        if (current.starts_with(ignore)) return true;
    }
    return false;
}

// helper to doublecode sectors for function calcDirHashes
void Hasher::processFileEntry(const std::filesystem::directory_entry& entry) {
    try {
        if (!std::filesystem::is_regular_file(entry.path())) return;
        if (shouldIgnoreDir(entry.path())) return;
        std::string wfile = entry.path().string();
        table[wfile] = calcHash(wfile);
    } catch (std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << '\n';
    } catch (std::exception& e) {
        std::cerr << "Error processing file " << entry.path() << ": " << e.what() << '\n';
    }
}

void Hasher::calcDirHashes(const std::string& current_path) {
    if (!std::filesystem::exists(current_path))
        throw std::runtime_error("Path doesn't exist: " + current_path);
    if (!std::filesystem::is_directory(current_path))
        throw std::runtime_error("Path isn't directory: " + current_path);
    if (recursive) {
        for (const auto& file : std::filesystem::recursive_directory_iterator(current_path)) {
            processFileEntry(file);
        }
    } else {
        for (const auto& file : std::filesystem::directory_iterator(current_path)) {
            processFileEntry(file);
        }
    }
}

void Hasher::loadBaseline(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("The baseline.json wasn't opened");
    json j;
    file >> j;
    baseline.clear();
    for (const auto& pair : j.items()) {
        baseline[pair.key()] = pair.value().get<uint64_t>();
    }
}

void Hasher::initHashes(const Config& conf, const std::string& path) {
    calcDirHashes(path);
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

void Hasher::deleteHash(const std::string& path, Logger& logger) {
    auto it = table.find(path);
    if (it != table.end()) {
        table.erase(it);
        logger.log(LOG_INFO, "Deleted: " + path);
    }
}
