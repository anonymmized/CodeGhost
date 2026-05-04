#include <xxhash.h>
#include <fstream>
#include <iostream>
#include <iomanip>

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
