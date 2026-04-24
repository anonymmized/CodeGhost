#include "watch_registry.hpp"
#include "utils.hpp"
#include <system_error>
#include <iostream>
#include <cstring>
#include <cerrno>

void WatchRegistry::addWatch(const std::string& path) {
    for (const auto& [_, p] : wd_to_path) {
        if (p == path) return;
    }
    int wd = inotify_add_watch(fd, path.c_str(), IN_CLOSE_WRITE | IN_CREATE | IN_MOVED_TO | IN_DELETE | IN_DELETE_SELF | IN_MOVED_FROM);
    if (wd >= 0) {
        wd_to_path[wd] = path;
    } else {
        std::cerr << "Failed to add watch for " << path << ": " << strerror(errno) << '\n';
    }
}

void WatchRegistry::addWatchRecursive(const std::string& root) {
    addWatch(root);
    std::error_code ec;
    for (const auto& entry : fs::recursive_directory_iterator(root, ec)) {
        if (ec) continue;
        if (!entry.is_directory()) continue;
        const std::string path = entry.path().string();
        auto name = entry.path().filename().string();
        if (shouldIgnoreFile(name)) continue;
        addWatch(path);
    }
}

void WatchRegistry::removeSubtree(const std::string& path, int wd) {
        remove(wd);
        for (auto  it = wd_to_path.begin(); it != wd_to_path.end(); ) {
            if (it->second.starts_with(path + "/")) {
                inotify_rm_watch(fd, it->first);
                it = wd_to_path.erase(it);
            } else {
                ++it;
            }
        }
}

void WatchRegistry::cleanup() {
    for (const auto& [wd, _] : wd_to_path) {
        inotify_rm_watch(fd, wd);
    }
    wd_to_path.clear();
}

void WatchRegistry::remove(int wd) {
    inotify_rm_watch(fd, wd);
    wd_to_path.erase(wd);
}

std::string WatchRegistry::getPath(int wd) {
    auto it_wd = wd_to_path.find(wd);
    if (it_wd == wd_to_path.end()) {
        return "";
    }
    return it_wd->second;
}
