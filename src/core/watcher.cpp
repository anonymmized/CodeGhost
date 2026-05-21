#include "./daemon.hpp"
#include "./watcher.hpp"
#include "./runtime_constants.hpp"
#include "../utils/utils.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <sys/inotify.h>
#include <stdexcept>

void Watcher::addWatch(const std::string& path) {
    int wd = inotify_add_watch(main_fd, path.c_str(), runtime::WATCH_MASK);
    if (wd < 0)
        throw std::runtime_error("Failed to add watch for path: " + path);
    watch_table[wd] = path;
}

void Watcher::init_fd() {
    main_fd = inotify_init();
    if (main_fd < 0)
        throw std::runtime_error("Failed to inirialize inotify");
}
int Watcher::getFd() { return main_fd; }
bool Watcher::hasWatch(int wd) {
    auto it = watch_table.find(wd);
    if (it == watch_table.end()) {
        return false;
    }
    return true;
}

void Watcher::registerRecursive(const std::string& fpath) {
    try {
        if (shouldIgnoreTree(fpath, config.ignore_paths)) return;
        addWatch(fpath);

        std::filesystem::recursive_directory_iterator it(fpath, std::filesystem::directory_options::skip_permission_denied);

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
    if (it != watch_table.end()) {
        inotify_rm_watch(main_fd, wd);
        watch_table.erase(wd);
    }
}

std::unordered_map<int, std::string> Watcher::getWatchTable() { return watch_table; }
