#pragma once

#include <sys/inotify.h>
#include <vector>
#include <string>
#include <unordered_map>
#include "daemon.hpp"

// class for fast and complex recursive watch theough the directories
// needs to keep wd and filenames in one place (watch_table) to use them in events processing
class Watcher {
    private:
        const Config& config;
        std::unordered_map<int, std::string> watch_table;
        int main_fd = -1;
    public:
        Watcher(const Config& _config) : config(_config) {
            init_fd();
        }
        void init_fd();
        void registerRecursive(const std::string& path);
        std::string getFullPath(int wd, const std::string& filename);
        void addWatch(const std::string& path);
        int getFd();
        bool hasWatch(int wd);
        void removeWatcher(int wd);
};
