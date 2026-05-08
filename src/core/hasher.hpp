#pragma once

#include <unordered_map>
#include <string>

uint64_t calcHash(const std::string& path);
bool compareHashes(const uint64_t& old_hash, const std::string& path);
void calcCurrent(std::unordered_map<std::string, uint64_t>& table, const std::string& current_path);
void calcRecursive(std::unordered_map<std::string, uint64_t>& table, const std::string& starting_path);
