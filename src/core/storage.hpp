#pragma once
#include <string>
#include <vector>
#include <mutex>
#include "indexer.hpp"

extern const std::string storage_path;
extern std::mutex storage_mutex;
struct RecordHeader {
    uint32_t size;
    int64_t timestamp;
    uint64_t hash;
    uint64_t block_index;
};

std::vector<Change> storage_read();
void storage_append(const std::vector<Change>& changes);
