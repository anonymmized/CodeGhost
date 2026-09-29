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

    if (interval < 1 || interval > 86400) {
        throw std::invalid_argument("scan_interval_seconds must be between 1 and 86400");
    }

    Config config;
    config.recursive = recursive.get<bool>();
    config.scanInterval = std::chrono::seconds{interval.get<int>()};

    for (const auto& item : ignored) {
        if (!item.is_string()) {
            throw std::invalid_argument("Each ignore_paths entry must be a string");
        }
        auto text = item.get<std::string>();
        if (text.empty()) {
            throw std::invalid_argument("ignore_paths entries must not be empty");
        }
        config.ignorePaths.push_back(std::filesystem::path{text});
    }
    return config;
}

void saveConfig(const Config& config, const std::filesystem::path& dest) {
    nlohmann::json data;
    data["recursive"] = config.recursive;
    data["scanInterval"] = config.scanInterval.count();
    data["ignorePaths"] = nlohmann::json::array();
    for (const auto& path : config.ignorePaths) {
        data["ignorePaths"].push_back(path.string());
    }

    std::ofstream configFile(dest);
    if (!configFile.is_open()) {
        throw std::runtime_error("Cannot open config: " + dest.string());
    }
    configFile << data.dump(4) << '\n';
    configFile.close();

    if (!configFile) {
        throw std::runtime_error("Failed to write config: " + dest.string());
    }
}
