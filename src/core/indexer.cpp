#include <fstream>
#include <vector>
#include <xxhash.h>
#include <string>
#include <algorithm>
#include <iostream>
#include <unordered_map>

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

uint64_t hashBlock(const std::string& block) {
    return XXH3_64bits(block.data(), block.size());
}

static std::unordered_map<std::string, FileState> state;

std::vector<std::string> makeBlocks(const std::vector<std::string>& lines, size_t block_size) {
    std::vector<std::string> blocks;
    for (size_t i = 0; i < lines.size(); i += block_size) {
        std::string block;
        for (size_t j = i; j < i + block_size && j < lines.size(); j++) {
            block += lines[j];
            block += '\n';
        }
        blocks.push_back(block);
    }
    return blocks;
}

std::vector<Change> mainIndexer(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        return {};
    }
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(infile, line)) lines.push_back(line);
    infile.close();
    std::vector<std::string> blocks = makeBlocks(lines, 5);
    std::vector<uint64_t> new_hashes;
    new_hashes.reserve(blocks.size());
    for (const auto& b : blocks) {
        new_hashes.push_back(hashBlock(b));
    }
    auto& old = state[filename];
    std::vector<Change> changes;
    size_t max_len = std::max(old.hashes.size(), new_hashes.size());
    for (size_t i = 0; i < max_len; ++i) {
        uint64_t old_h = (i < old.hashes.size()) ? old.hashes[i] : 0;
        uint64_t new_h = (i < new_hashes.size()) ? new_hashes[i] : 0;
        if (old_h != new_h) {
            Change c;
            c.file = filename;
            c.block_index = i;
            c.old_content = (i < old.blocks.size()) ? old.blocks[i] : "";
            c.new_content = (i < blocks.size()) ? blocks[i] : "";
            changes.push_back(std::move(c));
        }
    }
    state[filename] = {new_hashes, blocks};
        return changes;
}
