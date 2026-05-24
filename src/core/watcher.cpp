#include "./watcher.hpp"

#include "./runtime_constants.hpp"
#include "../utils/utils.hpp"

#include <stdexcept>
#include <string>
#include <vector>

void Watcher::addWatch(const std::string& path) {
    if (hasPath(path)) return;

    int wd = inotify_add_watch(main_fd, path.c_str(), runtime::WATCH_MASK);
    if (wd < 0) {
        throw std::runtime_error("Failed to add watch for path: " + path);
    }

    watch_table[wd] = path;
    path_table[path] = wd;
}

void Watcher::init_fd() {
    main_fd = inotify_init();
    if (main_fd < 0) {
        throw std::runtime_error("Failed to initialize inotify");
    }
}

int Watcher::getFd() { return main_fd; }

bool Watcher::hasWatch(int wd) {
    return watch_table.find(wd) != watch_table.end();
}

bool Watcher::hasPath(const std::string& path) const {
    return path_table.find(path) != path_table.end();
}

void Watcher::registerRecursive(const std::string& root_path) {
    try {
        if (shouldIgnoreTree(root_path, config.ignore_paths)) return;
        addWatch(root_path);

        std::filesystem::recursive_directory_iterator it(
            root_path,
            std::filesystem::directory_options::skip_permission_denied
        );

        for (const auto& entry : it) {
            std::error_code ec;
            if (!entry.is_directory(ec) || ec) continue;
            if (shouldIgnoreTree(entry.path(), config.ignore_paths)) continue;

            try {
                addWatch(entry.path().string());
            } catch (const std::exception&) {
                continue;
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        return;
    }
}

std::string Watcher::getFullPath(int wd, const std::string& filename) {
    auto it = watch_table.find(wd);
    if (it == watch_table.end()) return "";
    if (filename.empty()) return it->second;
    return it->second + "/" + filename;
}

void Watcher::removeWatcher(int wd) {
    auto it = watch_table.find(wd);
    if (it == watch_table.end()) return;

    inotify_rm_watch(main_fd, wd);
    path_table.erase(it->second);
    watch_table.erase(it);
}

void Watcher::removeWatchPath(const std::string& path) {
    auto it = path_table.find(path);
    if (it == path_table.end()) return;
    removeWatcher(it->second);
}

void Watcher::removeSubtree(const std::string& root_path) {
    std::vector<std::string> to_remove;
    for (const auto& [wd, path] : watch_table) {
        if (path == root_path || path.starts_with(root_path + "/")) {
            to_remove.push_back(path);
        }
    }

    for (const auto& path : to_remove) {
        removeWatchPath(path);
    }
}

void Watcher::renameSubtree(const std::string& old_root, const std::string& new_root) {
    std::vector<std::pair<int, std::string>> updates;
    for (const auto& [wd, path] : watch_table) {
        if (path == old_root || path.starts_with(old_root + "/")) {
            std::string suffix = path.substr(old_root.size());
            updates.push_back({wd, new_root + suffix});
        }
    }

    for (const auto& [wd, new_path] : updates) {
        path_table.erase(watch_table[wd]);
        watch_table[wd] = new_path;
        path_table[new_path] = wd;
    }
}

const std::unordered_map<int, std::string>& Watcher::getWatchTable() const {
    return watch_table;
}
