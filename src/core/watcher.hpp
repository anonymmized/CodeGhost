#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <chrono>

bool shouldIgnoreFile(const std::string& name);

class Watcher {
    public:
        struct PendingMove {
            std::string path;
            std::chrono::steady_clock::time_point ts;
        };
        explicit Watcher(std::string path_to_watch);
        ~Watcher();
        void start();
        void stop();
        using EventCallback = std::function<void(const std::string& file_path, const std::string& event_type)>;
        void setCallback(EventCallback cb);
    private:
        std::string watch_path;
        std::atomic<bool> running{false};
        EventCallback callback;
        std::mutex callback_mutex;
        std::thread worker;
};

class WatchRegistry {
    public:
        explicit WatchRegistry(int n_fd) : fd(n_fd) {}
        void addWatch(const std::string& path);
        void addWatchRecursive(const std::string& root);
        std::string getPath(int wd);
        std::string remove(int wd);
        void removeSubtree(const std::string& path, int wd);
        void cleanup();
    private:
        int fd;
        std::unordered_map<int, std::string> wd_to_path;
};
