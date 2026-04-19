#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct Change {
    std::string file;
    size_t block_index;
    std::string old_content;
    std::string new_content;
};

struct FileState {
    std::vector<uint64_t> hashes;
    std::vector<std::string> blocks;
};

uint64_t hashBlock(const std::string& block);

std::vector<std::string> makeBlocks(const std::vector<std::string>& lines, size_t block_size);
std::vector<Change> mainIndexer(const std::string& filename);
