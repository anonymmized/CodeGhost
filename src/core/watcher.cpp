#include "daemon.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <sys/inotify.h>
#include "../utils/utils.hpp"

void Watcher::addWatch(const std::string& path) {
    int wd = inotify_add_watch(main_fd, path, IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO | IN_ATTRIB);
    watch_table[wd] = path;
}

void Watcher::init_fd() { main_fd = inotify_init(); }
int Watcher::getFd() { return main_fd; }
bool Watcher::hasWatch(int wd) {
    auto it = watch_table.find(wd);
    if (it == watch_table.end()) {
        return false;
    }
    return true;
}

void Watcher::registerRecursive(const std::string& fpath) {
    if (shouldIgnoreTree(fpath, config.ignore_paths)) return;
    addWatch(fpath);
    for (const auto& path : std::filesystem::recursive_directory_iterator(fpath)) {
        if (!std::filesystem::is_directory(path)) continue;
        if (shouldIgnoreTree(path, config.ignore_paths)) continue;
        addWatch(path.path().string());
    }
}
std::string Watcher::getFullPath(int wd, const std::string& filename) {
    auto it = watch_table.find(wd);
    if (t == watch_table.end()) return "";
    if (filename.empty()) return it->second;
    return it->second + "/" + filename;
}
