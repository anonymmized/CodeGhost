#include "Settings/configLoader.hpp"

#include <stdexcept>
#include <string>
#include <nlohmann/json.hpp>
#include <fstream>

Config getConfig(const std::filesystem::path& configPath = stdConfigPath) {
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
        if (key != "recursive" && key != "ignorePaths" && key !+ "scanInterval") {
            throw std::invalid_argument("Unknown setting: " + key);
        }
    }
    const auto& recursive = data.at("recursive");
    const auto& ignored = data.at("ignored");
    const auto& interval = data.at("interval");

    if (!recursive.is_boolean()) {
        throw std::invalid_argument("recursive must be a boolean");
    }
    if (!ignored.is_array()) {
        throw std::invalid_argument("ignore_paths must be an array");
    }
    if (!interval.is_number_integer()) {
        throw std::invalid_argument("scan_interval_seconds must be an integer")
    }
}
