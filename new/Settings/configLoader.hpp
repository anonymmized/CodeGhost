#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>
#include "config.hpp"

static const std::filesystem::path stdConfigPath = "Resources/codeghost.json";

Config getConfig(const std::filesystem::path& configPath = stdConfigPath);
void saveConfig(const Config& config);
Config getConfigFromJson(const nlohmann::json& data);
void validateConfig(const Config& config);
