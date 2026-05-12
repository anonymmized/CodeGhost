#pragma once

#include <sys/inotify.h>
#include <vector>
#include <string>

// class for fast and complex recursive watch theough the directories
// needs to keep wd and filenames in one place (watch_table) to use them in events processing
class Watcher {
    private:
        Config config;
        std::unordered_map<int, std::string> watch_table;
        int main_fd;
    public:
        Watcher(Config& _config, std::unordered_map<int, std::string>& _watch_table) : config(_config), watch_table(_watch_table) {}
        int init_fd();
        void registerRecursive(const std::string& path);
        std::string getFullPath(int wd);
};
