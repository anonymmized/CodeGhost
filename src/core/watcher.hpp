#pragma once

#include <string>
#include <unordered_map>

#include "daemon.hpp"
#include "inotify_compat.hpp"

class Watcher {
private:
    const Config& config;
    std::unordered_map<int, std::string> watch_table;
    std::unordered_map<std::string, int> path_table;
    int main_fd = -1;

public:
    explicit Watcher(const Config& _config) : config(_config) { init_fd(); }

    void init_fd();
    void registerRecursive(const std::string& path);
    std::string getFullPath(int wd, const std::string& filename);
    void addWatch(const std::string& path);
    int getFd();
    bool hasWatch(int wd);
    bool hasPath(const std::string& path) const;
    void removeWatcher(int wd);
    void removeWatchPath(const std::string& path);
    void removeSubtree(const std::string& root_path);
    void renameSubtree(const std::string& old_root, const std::string& new_root);
    const std::unordered_map<int, std::string>& getWatchTable() const;
};
