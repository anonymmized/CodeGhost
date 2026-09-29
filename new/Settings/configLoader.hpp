#pragma once

#include <filesystem>
#include "config.hpp"

static const std::filesystem::path stdConfigPath = "Resources/codeghost.json";

Config getConfig(const std::filesystem::path& configPath = stdConfigPath);
void saveConfig(const Config& config, const std::filesystem::path& dest = stdConfigPath);
