#include "hasher.hpp"

#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>
#include <xxhash.h>

using json = nlohmann::ordered_json;

std::optional<FileState> Hasher::readFileState(const std::string& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec) return std::nullopt;
    if (!std::filesystem::is_regular_file(path, ec) || ec) return std::nullopt;

    FileState state;
    state.hash = calcHash(path);

    ec.clear();
    state.size = std::filesystem::file_size(path, ec);
    if (ec) state.size = 0;

    ec.clear();
    state.permissions = static_cast<uint32_t>(std::filesystem::status(path, ec).permissions());
    if (ec) state.permissions = 0;

    ec.clear();
    state.write_time_ticks = std::filesystem::last_write_time(path, ec).time_since_epoch().count();
    if (ec) state.write_time_ticks = 0;

    return state;
}

void Hasher::recordFileState(const std::string& path) {
    auto state = readFileState(path);
    if (state.has_value()) {
        table[path] = *state;
    }
}

bool Hasher::isCriticalPath(const std::filesystem::path& path) {
    std::string current = path.string();
    for (const auto& critical : critical_paths) {
        if (current.starts_with(critical)) return true;
    }
    return false;
}

LogLevel Hasher::levelForPath(const std::filesystem::path& path, LogLevel critical_level) {
    return isCriticalPath(path) ? critical_level : LOG_INFO;
}

void Hasher::fileChanged(const std::string& path, Logger& logger) {
    auto state = readFileState(path);
    if (!state.has_value()) return;

    auto baseline_it = baseline.find(path);
    if (baseline_it == baseline.end()) {
        table[path] = *state;
        logger.log(levelForPath(path, LOG_WARN), "Created: " + path);
        return;
    }

    if (baseline_it->second.hash != state->hash) {
        table[path] = *state;
        logger.log(levelForPath(path, LOG_WARN), "Modified: " + path);
        return;
    }

    if (table.find(path) == table.end()) {
        table[path] = *state;
    }
}

void Hasher::fileAttributed(const std::string& path, Logger& logger) {
    auto state = readFileState(path);
    if (!state.has_value()) return;

    auto runtime_it = table.find(path);
    auto baseline_it = baseline.find(path);
    const FileState* reference = nullptr;

    if (runtime_it != table.end()) {
        reference = &runtime_it->second;
    } else if (baseline_it != baseline.end()) {
        reference = &baseline_it->second;
    }

    if (reference == nullptr) {
        table[path] = *state;
        logger.log(levelForPath(path, LOG_WARN), "Created after metadata event: " + path);
        return;
    }

    bool content_changed = reference->hash != state->hash;
    bool metadata_changed = reference->size != state->size ||
                            reference->permissions != state->permissions ||
                            reference->write_time_ticks != state->write_time_ticks;

    if (!content_changed && !metadata_changed) return;

    table[path] = *state;
    LogLevel level = levelForPath(path, content_changed ? LOG_WARN : LOG_INFO);
    if (content_changed && metadata_changed) {
        logger.log(level, "Content and metadata changed: " + path);
    } else if (content_changed) {
        logger.log(level, "Modified after metadata event: " + path);
    } else {
        logger.log(level, "Metadata changed: " + path);
    }
}

uint64_t Hasher::calcHash(const std::string& path) {
    std::ifstream infile(path, std::ios::binary);
    if (!infile.is_open()) {
        throw std::runtime_error("File wasn't opened");
    }

    XXH64_state_t* state = XXH64_createState();
    if (!state) {
        throw std::runtime_error("Failed to allocate state");
    }

    XXH64_reset(state, 0);
    char buffer[8192];
    while (infile.read(buffer, sizeof(buffer)) || infile.gcount() > 0) {
        XXH64_update(state, buffer, infile.gcount());
    }

    uint64_t hash = XXH64_digest(state);
    XXH64_freeState(state);
    return hash;
}

void Hasher::loadBaselineFile(const std::string& path) {
    loadBaseline(path);
    table = baseline;
}

void Hasher::syncBaseline(const std::string& path) {
    baseline = table;
    saveBaseline(path);
}

void Hasher::reloadRuntime(const Config& conf) {
    initHashes(conf);
}

void Hasher::resetRuntimeToBaseline() {
    table = baseline;
}

bool Hasher::shouldIgnoreDir(const std::filesystem::path& path) {
    std::string current = path.string();
    for (const auto& ignore : ignore_paths) {
        if (current.starts_with(ignore)) return true;
    }
    return false;
}

void Hasher::processFileEntry(const std::filesystem::directory_entry& entry) {
    try {
        std::error_code ec;
        if (!entry.is_regular_file(ec) || ec) return;
        if (shouldIgnoreDir(entry.path())) return;

        recordFileState(entry.path().string());
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << '\n';
    } catch (const std::exception& e) {
        std::cerr << "Error processing file " << entry.path() << ": " << e.what() << '\n';
    }
}

void Hasher::calcDirHashes(const std::string& current_path) {
    std::error_code ec;
    if (!std::filesystem::exists(current_path, ec) || ec) return;
    if (!std::filesystem::is_directory(current_path, ec) || ec) return;

    try {
        if (recursive) {
            std::filesystem::recursive_directory_iterator it(
                current_path,
                std::filesystem::directory_options::skip_permission_denied
            );
            for (const auto& file : it) {
                processFileEntry(file);
            }
        } else {
            std::filesystem::directory_iterator it(
                current_path,
                std::filesystem::directory_options::skip_permission_denied
            );
            for (const auto& file : it) {
                processFileEntry(file);
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        return;
    }
}

void Hasher::loadBaseline(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("The baseline.json wasn't opened");
    }

    json j;
    file >> j;
    baseline.clear();
    for (const auto& pair : j.items()) {
        if (pair.value().is_number_unsigned()) {
            baseline[pair.key()] = FileState{pair.value().get<uint64_t>(), 0, 0, 0};
            continue;
        }

        FileState state;
        state.hash = pair.value().value("hash", 0ULL);
        state.size = pair.value().value("size", 0ULL);
        state.permissions = pair.value().value("permissions", 0U);
        state.write_time_ticks = pair.value().value("write_time_ticks", 0LL);
        baseline[pair.key()] = state;
    }
}

void Hasher::saveBaseline(const std::string& baseline_path) {
    std::ofstream baselinef(baseline_path);
    if (!baselinef.is_open()) {
        throw std::runtime_error("The baseline.json wasn't opened");
    }

    json j;
    for (const auto& pair : table) {
        j[pair.first] = {
            {"hash", pair.second.hash},
            {"size", pair.second.size},
            {"permissions", pair.second.permissions},
            {"write_time_ticks", pair.second.write_time_ticks}
        };
    }

    baselinef << j.dump(4);
}

void Hasher::initHashes(const Config& conf) {
    table.clear();
    for (const auto& path : conf.watch_paths) {
        calcDirHashes(path);
    }
}

void Hasher::deleteHash(const std::string& path, Logger& logger) {
    auto it = table.find(path);
    if (it == table.end()) return;

    table.erase(it);
    logger.log(levelForPath(path, LOG_ERROR), "Deleted: " + path);
}

void Hasher::deletePathTree(const std::string& path, Logger& logger) {
    std::vector<std::string> to_delete;
    for (const auto& [entry_path, state] : table) {
        if (entry_path == path || entry_path.starts_with(path + "/")) {
            to_delete.push_back(entry_path);
        }
    }

    for (const auto& entry_path : to_delete) {
        deleteHash(entry_path, logger);
    }
}

void Hasher::registerPathTree(const std::string& path, Logger& logger) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec) return;

    if (std::filesystem::is_regular_file(path, ec) && !ec) {
        fileChanged(path, logger);
        return;
    }

    if (!std::filesystem::is_directory(path, ec) || ec) return;
    if (shouldIgnoreDir(path)) return;

    if (recursive) {
        std::filesystem::recursive_directory_iterator it(
            path,
            std::filesystem::directory_options::skip_permission_denied
        );
        for (const auto& entry : it) {
            processFileEntry(entry);
        }
    } else {
        std::filesystem::directory_iterator it(
            path,
            std::filesystem::directory_options::skip_permission_denied
        );
        for (const auto& entry : it) {
            processFileEntry(entry);
        }
    }
}

void Hasher::movePathTree(const std::string& old_path, const std::string& new_path, Logger& logger, bool is_directory) {
    LogLevel level = (isCriticalPath(old_path) || isCriticalPath(new_path)) ? LOG_WARN : LOG_INFO;

    if (!is_directory) {
        auto state = readFileState(new_path);
        table.erase(old_path);
        if (state.has_value()) {
            table[new_path] = *state;
        }
        logger.log(level, "Moved: " + old_path + " -> " + new_path);
        return;
    }

    std::vector<std::pair<std::string, FileState>> moved_entries;
    std::vector<std::string> old_entries;
    for (const auto& [entry_path, state] : table) {
        if (entry_path == old_path || entry_path.starts_with(old_path + "/")) {
            std::string suffix = entry_path.substr(old_path.size());
            moved_entries.push_back({new_path + suffix, state});
            old_entries.push_back(entry_path);
        }
    }

    for (const auto& entry_path : old_entries) {
        table.erase(entry_path);
    }
    for (const auto& [entry_path, state] : moved_entries) {
        table[entry_path] = state;
    }
    registerPathTree(new_path, logger);
    logger.log(level, "Moved: " + old_path + " -> " + new_path);
}

void Hasher::approveFile(const std::string& path) {
  auto it = table.find(path);
  if (it == table.end()) return;
  baseline[path] = it->second;
}
