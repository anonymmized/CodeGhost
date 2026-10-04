#pragma once

#include "Inspector/types.hpp"

#include <filesystem>
#include <vector>

void saveBaseline(const std::filesystem::path& storagePath, const ScanPoint& scanResult);
std::vector<FileRecord> loadBaseline(const std::filesystem::path& storagePath);
