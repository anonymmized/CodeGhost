#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <mutex>

struct Change {
    int64_t timestamp;
    uint64_t hash;
    size_t block_index;
    std::string file;
    std::string old_content;
    std::string new_content;
};

struct FileState {
    std::vector<uint64_t> hashes;
    std::vector<std::string> blocks;
};

class Indexer {
    public:
        Indexer(size_t bl_size) : block_size(std::move(bl_size)) {}
        std::vector<Change> process(const std::string& filename);
        void remove(const std::string& path);
        void rename(const std::string& old_path, const std::string& new_path);
    private:
        size_t block_size;
        std::unordered_map<std::string, FileState> state;
        std::mutex state_mutex;
};

uint64_t hashBlock(const std::string& block);

std::vector<std::string> makeBlocks(const std::vector<std::string>& lines, size_t block_size);
