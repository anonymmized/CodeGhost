#pragma once

#include <unordered_map>
#include <sys/inotify.h>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

class WatchRegistry {
    private:
        int fd;
        std::unordered_map<int, std::string> wd_to_path;
    public:
        explicit WatchRegistry(int n_fd) : fd(n_fd) {}
        void addWatch(const std::string& path);
        void addWatchRecursive(const std::string& root);
        void removeSubtree(const std::string& path, int wd);
        void cleanup();
        void remove(int wd);
        std::string getPath(int wd);
};
