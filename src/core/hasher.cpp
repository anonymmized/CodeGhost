//#include <xxhash.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <unordered_map>

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

void calcRecursive(std::unordered_map<std::string, uint64_t>& table, const std::string& starting_path) {
    if (!std::filesystem::exists(starting_path))
        throw std::runtime_error("Path doesn't exist: " + starting_path);
    if (!std::filesystem::is_directory(starting_path))
        throw std::runtime_error("Path isn't directory: " + starting_path);
    for (const auto& file : std::filesystem::recursive_directory_iterator(starting_path)) {
        try {
            if (std::filesystem::is_regular_file(file.path())) {
                continue;
            }
            std::string wfile = file.path().string();
            table[wfile] = calcHash(wfile);
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Filesystem error: " << e.what() << '\n';
        } catch (const std::exception& e) {
            std::cerr << "Error processing file " << wfile << ": " << e.what() << '\n';
        }
    }
}
