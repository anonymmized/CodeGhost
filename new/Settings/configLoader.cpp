#include "Settings/configLoader.hpp"

#include <stdexcept>
#include <string>
#include <nlohmann/json.hpp>
#include <fstream>

Config getConfig(const std::filesystem::path& configPath) {
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        throw std::runtime_error("Cannot open config: " + configPath.string());
    }

    nlohmann::json data;
    configFile >> data;
    if (!data.is_object()) {
        throw std::invalid_argument("Config must be a JSON object");
    }
    for (const auto& [key, value] : data.items()) {
        if (key != "recursive" && key != "ignorePaths" && key != "scanInterval") {
            throw std::invalid_argument("Unknown setting: " + key);
        }
    }
    Config config = getConfigFromJson(data);
    validateConfig(config);
    return config;
}

Config getConfigFromJson(const nlohmann::json& data) {
    const auto& recursive = data.at("recursive");
    const auto& ignored = data.at("ignorePaths");
    const auto& interval = data.at("scanInterval");

    if (!recursive.is_boolean()) {
        throw std::invalid_argument("recursive must be a boolean");
    }
    if (!ignored.is_array()) {
        throw std::invalid_argument("ignore_paths must be an array");
    }
    if (!interval.is_number_integer()) {
        throw std::invalid_argument("scan_interval_seconds must be an integer");
    }
    Config config;
    config.recursive = recursive.get<bool>();
    if (!interval.is_number_unsigned() && interval.get<std::int64_t>() < 0) {
        throw std::invalid_argument("scanInterval must not be negative");
    }
    config.scanInterval = interval.get<std::uint64_t>();
    for (const auto& item : ignored) {
        if (!item.is_string()) {
            throw std::invalid_argument("Each ignorePaths entry must be a string");
        }
        config.ignorePaths.push_back(item.get<std::filesystem::path>());
    }
    return config;
}

void saveConfig(const Config& config) {
    validateConfig(config);
    nlohmann::json data;
    data["recursive"] = config.recursive;
    data["scanInterval"] = config.scanInterval;
    data["ignorePaths"] = nlohmann::json::array();
    for (const auto& path : config.ignorePaths) {
        data["ignorePaths"].push_back(path.string());
    }

    std::ofstream configFile(stdConfigPath);
    if (!configFile.is_open()) {
        throw std::runtime_error("Cannot open config: " + stdConfigPath.string());
    }
    configFile << data.dump(4) << '\n';
    configFile.close();

    if (!configFile) {
        throw std::runtime_error("Failed to write config: " + stdConfigPath.string());
    }
}

void validateConfig(const Config& config) {
    if (config.scanInterval < 1 || config.scanInterval > 86400) {
        throw std::invalid_argument("scan_interval_seconds must be between 1 and 86400");
    }
    for (const auto& item : config.ignorePaths) {
        if (item.empty()) {
            throw std::invalid_argument("Each ignore_paths entry must not be empty");
        }
    }
}
