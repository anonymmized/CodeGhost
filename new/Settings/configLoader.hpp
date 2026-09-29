#pragma once

#include <filesystem>

static const std::filesystem::path stdConfigPath = "Resources/codeghost.json";

Config getConfig(const std::filesystem::path& configPath = stdConfigPath);
