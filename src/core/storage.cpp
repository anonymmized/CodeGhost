#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include <ctime>
#include "indexer.hpp"
#include <vector>
#include <mutex>
#include <filesystem>
namespace fs = std::filesystem;

static const std::string storage_path = ".codeghost/history.bin";
static std::mutex storage_mutex;

struct RecordHeader {
    uint32_t size;
    int64_t timestamp;
    uint64_t hash;
    uint64_t block_index;
};

std::vector<Change> storage_read(){
        std::ifstream s(storage_path, std::ios::binary);
        if (!s.is_open()) throw std::runtime_error("storage open failed");
        std::vector<Change> changes;
        while (true) {
            RecordHeader header;
            s.read(reinterpret_cast<char*>(&header), sizeof(header));
            if (!s) break;
            std::vector<char> buffer(header.size);
            s.read(buffer.data(), header.size);
            if (!s) break;
            const char* ptr = buffer.data();
            auto read_cstr = [&](std::string& out) {
                out.clear();
                while (*ptr != '\0') {
                    out.push_back(*ptr++);
                }
                ++ptr;
            };
            std::string file, old_c, new_c;
            read_cstr(file);
            read_cstr(old_c);
            read_cstr(new_c);
            Change c;
            c.timestamp = header.timestamp;
            c.hash = header.hash;
            c.block_index = header.block_index;
            c.file = std::move(file);
            c.old_content = std::move(old_c);
            c.new_content = std::move(new_c);
            changes.push_back(std::move(c));
        }
        return changes;
}

void storage_append(const std::vector<Change>& changes){
    std::lock_guard<std::mutex> lock(storage_mutex);
    fs::create_directories(".codeghost");
    std::ofstream s(storage_path, std::ios::binary | std::ios::app);
    if (!s.is_open()) throw std::runtime_error("storage open failed");
    for ( const auto& c : changes) {
        uint32_t payload_size = c.file.size() + 1 + c.old_content.size() + 1 + c.new_content.size() + 1;
        RecordHeader header {
            payload_size,
            c.timestamp,
            c.hash,
            c.block_index
        };
        s.write(reinterpret_cast<const char*>(&header), sizeof(header));
        s.write(c.file.data(), c.file.size());
        s.put('\0');
        s.write(c.old_content.data(), c.old_content.size());
        s.put('\0');
        s.write(c.new_content.data(), c.new_content.size());
        s.put('\0');
    }
}
